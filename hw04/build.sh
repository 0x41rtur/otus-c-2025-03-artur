#!/bin/bash

# копируем репу
git clone https://github.com/curl/curl.git

# идем в клонированный каталог
cd curl

# автоконфиг из кробки
./buildconf

# выключаем все лишнее
./configure \
  --disable-shared \
  --enable-static \
  --with-openssl \
  --without-libpsl \
  --disable-ftp \
  --disable-file \
  --disable-ldap \
  --disable-ldaps \
  --disable-dict \
  --disable-gopher \
  --disable-smb \
  --disable-imap \
  --disable-pop3 \
  --disable-rtsp \
  --disable-smtp \
  --disable-mqtt \
  --disable-tftp \
  --disable-ws \
  --disable-wss \
  --disable-ipfs \
  --disable-ipns \
  --disable-libcurl-option \
  --without-zstd \
  --without-brotli \
  --without-nghttp2 \
  --without-ngtcp2 \
  --without-nghttp3

# собираем проект
make

./src/curl --version

#curl 8.14.0-DEV (aarch64-apple-darwin24.5.0) libcurl/8.14.0-DEV OpenSSL/3.5.0 zlib/1.2.12 libidn2/2.3.8
#Release-Date: [unreleased]
#Protocols: http https telnet ws wss
#Features: alt-svc AsynchDNS HSTS HTTPS-proxy IDN IPv6 Largefile libz NTLM SSL threadsafe TLS-SRP UnixSockets
