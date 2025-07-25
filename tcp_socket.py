# -*- coding: utf-8 -*-
import socket
import json

def main():
    HOST = '0.0.0.0'
    PORT = 9000

    print("[+] Listening on port", PORT)
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, PORT))
        s.listen(1)
        conn, addr = s.accept()
        with conn:
            print('[+] Connected by', addr)

            data = b''
            while True:
                part = conn.recv(1024)
                if not part:
                    break
                data += part

            try:
                frequencies = json.loads(data.decode())
            except Exception as e:
                print("[!] JSON parse error:", e)
                print(data.decode())
                return

            print("\nReceived frequency list:\n")
            for i, f in enumerate(frequencies):
                print(f"{i}: {f['freq_mhz']} MHz | gain = {f['gain']} | symbol = {f['symbol']} | preamble = {f['preamble']} | black_level = {f['black_level']}")

            print("\n=== Select index to jam ===")
            idx = input("Index: ").strip()
            try:
                idx = int(idx)
                freq = frequencies[idx]['freq_mhz']
                cmd = f"JAM {freq}\n"
                conn.sendall(cmd.encode())
                print(f"[>] Sent command: {cmd.strip()}")
            except:
                print("[!] Invalid index")

if __name__ == "__main__":
    main()
