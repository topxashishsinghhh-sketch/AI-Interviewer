#!/usr/bin/env python3
"""
AI Interviewer - Instant Live Web Launcher
Launches the web application with zero external dependencies and opens your browser.
"""

import http.server
import socketserver
import webbrowser
import os
import sys
import threading
import time

PORT = 3000
HOST = "localhost"

# Ensure server runs from the repository root
ROOT_DIR = os.path.dirname(os.path.abspath(__file__))
os.chdir(ROOT_DIR)

class CustomHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Prevent caching during live local development
        self.send_header('Cache-Control', 'no-cache, no-store, must-revalidate')
        self.send_header('Pragma', 'no-cache')
        self.send_header('Expires', '0')
        super().end_headers()

    def log_message(self, format, *args):
        # Keep terminal output clean
        if "404" in format % args:
            sys.stderr.write(f" [404] {format % args}\n")

def open_browser(port):
    time.sleep(0.7)
    url = f"http://{HOST}:{port}"
    print(f" >> Opening your browser at {url} ...\n")
    webbrowser.open(url)

def main():
    target_port = PORT
    for p in range(PORT, PORT + 20):
        try:
            with socketserver.TCPServer(("", p), CustomHandler) as httpd:
                print("\n" + "=" * 60)
                print("   AI INTERVIEWER - LIVE LOCAL ENVIRONMENT")
                print("=" * 60)
                print(f"  * Status       : Running")
                print(f"  * Local URL    : http://{HOST}:{p}")
                print(f"  * Direct App   : http://{HOST}:{p}/web/")
                print(f"  * Question Bank: 310 Questions (12 Core Subjects)")
                print("=" * 60)
                print("  Press Ctrl+C in this terminal window to stop.\n")

                threading.Thread(target=open_browser, args=(p,), daemon=True).start()
                httpd.serve_forever()
                break
        except OSError:
            continue

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nAI Interviewer server stopped.")
        sys.exit(0)
