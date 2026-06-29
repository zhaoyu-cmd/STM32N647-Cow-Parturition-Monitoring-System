"""
TCP server for the 991 calving alert demo.

Listens on 0.0.0.0:8080 for the STM32N6 board's detection JSON, then
prints a Chinese alert message that a herder can act on directly.

Run:
    python tcp_alert_server.py
    Ctrl+C to stop (responsive within ~0.5 s on Windows).

The board (Try2/Cow Parturition Monitoring System) is hard-coded to connect to
192.168.2.11:8080. PC NIC must be 192.168.2.11 / 255.255.255.0.
"""
import json
import socket
import time
import urllib.error
import urllib.request

HOST = "0.0.0.0"
PORT = 8080
ACCEPT_POLL_S = 0.5
RECV_POLL_S = 0.5

# PushPlus 个人推送密钥：https://www.pushplus.plus -> 一对一推送
# 注意：这是私人 token，如要复现，使用你自己的 token 替换下面的字符串。
PUSHPLUS_TOKEN = "使用自己的token"
PUSHPLUS_URL = "https://www.pushplus.plus/send"
PUSHPLUS_TIMEOUT_S = 3.0  # 短超时，避免推送阻塞 TCP 主循环

# 节流策略（绕开 PushPlus 免费版"推送频率过快" code 999）：
# 每发 PUSH_BURST_LIMIT 条进入 PUSH_BURST_PAUSE_S 秒静默期，
# 静默期到了之后计数器清零，可以继续发。
PUSH_BURST_LIMIT = 6
PUSH_BURST_PAUSE_S = 120.0
_push_counter = 0
_push_pause_until = 0.0  # time.monotonic() 之前不许推

def push_to_phone(title: str, content: str) -> None:
    """Best-effort 微信推送。失败只打日志，不影响 TCP 主流程。"""
    if not PUSHPLUS_TOKEN:
        print("[push] token 未配置，跳过")
        return
    payload = json.dumps({
        "token": PUSHPLUS_TOKEN,
        "title": title,
        "content": content,
    }).encode("utf-8")
    req = urllib.request.Request(
        PUSHPLUS_URL,
        data=payload,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=PUSHPLUS_TIMEOUT_S) as resp:
            body = resp.read().decode("utf-8", errors="replace")
        try:
            ret = json.loads(body)
            if ret.get("code") == 200:
                print(f"[push] OK -> 微信 (msgId={ret.get('data')})")
            else:
                print(f"[push] PushPlus 拒绝: {body}")
        except json.JSONDecodeError:
            print(f"[push] 响应非 JSON: {body}")
    except urllib.error.URLError as e:
        print(f"[push] 网络错误: {e}")
    except socket.timeout:
        print(f"[push] 超时 (>{PUSHPLUS_TIMEOUT_S}s)")
    except Exception as e:
        print(f"[push] 未知错误: {e}")


def format_alert(msg: dict) -> str:
    """Turn one detection JSON into a herder-friendly Chinese alert line.
    Only the action message is shown; class/conf intentionally omitted
    so the herder is not distracted by technical detail. """
    ts = time.strftime("%H:%M:%S")
    return f"[ALERT {ts}] 母牛即将生产，请前去牛棚！"


def handle_payload(raw: bytes):
    """Print one received chunk. Parse as JSON when possible."""
    try:
        text = raw.decode("utf-8", errors="strict")
    except UnicodeDecodeError:
        print(f"[server] [{time.strftime('%H:%M:%S')}] RX (hex): {raw.hex()}")
        return

    # The board can send multiple JSON lines in one TCP segment if traffic
    # bursts; split on newline so each gets its own alert.
    for line in text.splitlines():
        line = line.strip()
        if not line:
            continue
        # Try JSON first; fall back to raw for non-JSON traffic (e.g. HELLO).
        try:
            msg = json.loads(line)
        except json.JSONDecodeError:
            print(f"[server] [{time.strftime('%H:%M:%S')}] RX: {line}")
            continue
        if isinstance(msg, dict) and msg.get("event") == "detection":
            print(format_alert(msg))
            # 标题加 [HH:MM:SS]、正文加完整时间戳：避免 PushPlus 服务端
            # 把相同内容判为重复推送 (code 999 "请勿频繁推送相同内容")。
            now_short = time.strftime("%H:%M:%S")
            now_full = time.strftime("%Y-%m-%d %H:%M:%S")
            now_m = time.monotonic()
            global _push_counter, _push_pause_until
            if now_m < _push_pause_until:
                remaining = _push_pause_until - now_m
                print(f"[push] 静默期：还剩 {remaining:.0f}s，本次不推（控制台仍记录）")
            else:
                push_to_phone(
                    title=f"母牛即将生产 [{now_short}]",
                    content=f"请前去牛棚查看母牛生产情况！\n报警时间：{now_full}",
                )
                _push_counter += 1
                if _push_counter >= PUSH_BURST_LIMIT:
                    _push_pause_until = now_m + PUSH_BURST_PAUSE_S
                    _push_counter = 0
                    print(f"[push] 已达 {PUSH_BURST_LIMIT} 条，进入 {PUSH_BURST_PAUSE_S:.0f}s 静默")
        else:
            print(f"[server] [{time.strftime('%H:%M:%S')}] RX (json): {line}")


def serve_one_connection(conn, addr):
    print(f"[server] >>> board connected from {addr[0]}:{addr[1]} at {time.strftime('%H:%M:%S')}")
    conn.settimeout(RECV_POLL_S)
    try:
        while True:
            try:
                data = conn.recv(1024)
            except socket.timeout:
                continue
            if not data:
                print(f"[server] <<< board {addr[0]} disconnected")
                return
            handle_payload(data)
    except ConnectionResetError:
        print(f"[server] !!! board {addr[0]} reset the connection")


def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as srv:
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind((HOST, PORT))
        srv.listen(1)
        srv.settimeout(ACCEPT_POLL_S)
        print(f"[server] listening on {HOST}:{PORT} ...")
        print("[server] expecting board at 192.168.2.20 and PC NIC at 192.168.2.11")
        print("[server] press Ctrl+C to stop")
        print()
        while True:
            try:
                conn, addr = srv.accept()
            except socket.timeout:
                continue
            with conn:
                serve_one_connection(conn, addr)
            print()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n[server] bye")
