#! /bin/bash

openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes -subj "/C=HU/ST=Csongrad/L=Szeged/O=SZTE/OU=Uni/CN=www.u-szeged.hu";
openssl s_server -key key.pem -cert cert.pem -accept 8080 -www
