#!/usr/bin/env python3
"""Offline tests for the BBSlink legacy authentication exchange."""

import hashlib
import http.server
import os
import pathlib
import shlex
import subprocess
import tempfile
import threading

ROOT = pathlib.Path(__file__).resolve().parents[1]


class Handler(http.server.BaseHTTPRequestHandler):
    reject = False
    requests = []

    def log_message(self, *_args):
        pass

    def do_GET(self):
        self.__class__.requests.append((self.path, dict(self.headers.items())))
        if self.path.startswith('/token.php?key='):
            content = b'test-token'
            status = 200
        elif self.path.startswith('/auth.php?key='):
            header = self.headers
            checks = [
                header.get('X-System') == 'sys-test',
                header.get('X-User') == '42',
                header.get('X-Door') == 'menu',
                header.get('X-Rows') == '40',
                header.get('X-Type') == 'C',
                header.get('X-Token') == 'test-token',
                header.get('X-Auth') == hashlib.md5(b'auth-testtest-token').hexdigest(),
                header.get('X-Code') == hashlib.md5(b'scheme-testtest-token').hexdigest(),
                header.get('X-Key') == self.path.rsplit('=', 1)[1],
            ]
            content = b'complete' if all(checks) and not self.reject else b'rejected'
            status = 200
        else:
            content = b'not found'
            status = 404
        self.send_response(status)
        self.send_header('Content-Type', 'text/plain')
        self.end_headers()
        self.wfile.write(content)


def run():
    with tempfile.TemporaryDirectory() as d:
        tmp = pathlib.Path(d)
        credentials = tmp / 'credentials'
        credentials.write_text('syscode=sys-test\nauthcode=auth-test\nschemecode=scheme-test\n')
        credentials.chmod(0o600)
        telnet = tmp / 'telnet-stub'
        telnet.write_text('#!/bin/sh\nprintf "TELNET:%s:%s:%s\\n" "$1" "$2" "$TERM"\n')
        telnet.chmod(0o700)
        server = http.server.HTTPServer(('127.0.0.1', 0), Handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            binary = tmp / 'bbslink_client'
            flags = [
                f'-DBBSLINK_HOST="127.0.0.1"',
                f'-DBBSLINK_HTTP_BASE="http://127.0.0.1:{server.server_port}"',
                f'-DBBSLINK_TELNET_BIN="{telnet}"',
            ]
            cflags = shlex.split(subprocess.check_output(
                ['pkg-config', '--cflags', 'libcurl', 'openssl'], text=True))
            libs = shlex.split(subprocess.check_output(
                ['pkg-config', '--libs', 'libcurl', 'openssl'], text=True))
            subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                            *flags, *cflags, str(ROOT / 'bbslink_client.c'), '-o', str(binary),
                            *libs], check=True)
            env = dict(os.environ, TERM='vt100')
            base = [str(binary), '-c', str(credentials), '-r', '40', 'menu', '42']
            good = subprocess.run(base, capture_output=True, text=True, env=env)
            assert good.returncode == 0, good.stderr
            assert good.stdout == 'TELNET:127.0.0.1:23:vt100\n', good.stdout
            assert len(Handler.requests) == 2
            assert Handler.requests[0][0].startswith('/token.php?key=')
            assert Handler.requests[1][0].startswith('/auth.php?key=')
            assert Handler.requests[0][0].split('key=')[1] == Handler.requests[1][0].split('key=')[1]

            Handler.reject = True
            rejected = subprocess.run(base, capture_output=True, text=True, env=env)
            assert rejected.returncode == 1
            assert 'authentication rejected' in rejected.stderr
            assert 'TELNET:' not in rejected.stdout
            Handler.reject = False

            credentials.chmod(0o644)
            exposed = subprocess.run(base, capture_output=True, text=True, env=env)
            assert exposed.returncode == 1
            assert 'private regular file' in exposed.stderr
            credentials.chmod(0o600)

            invalid = subprocess.run([str(binary), '-c', str(credentials), 'menu', 'xx'],
                                     capture_output=True, text=True, env=env)
            assert invalid.returncode == 2
            assert subprocess.run([str(binary), '-h'], stdout=subprocess.DEVNULL).returncode == 0
            print('PASS: auth headers, token key, Telnet exec, rejection, file mode, arguments')
        finally:
            server.shutdown()
            thread.join(timeout=3)
            server.server_close()


if __name__ == '__main__':
    run()
