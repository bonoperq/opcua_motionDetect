## Generate Key and CA cert
openssl req -x509 -nodes -newkey rsa:4096 -keyout ca.key -out ca.crt -days 7300 -subj "/CN=MyCA/OU=Elsys/O=France/L=Antibes/ST=France/C=FR" -addext "keyUsage = digitalSignature, keyEncipherment, cRLSign, keyCertSign" -addext "extendedKeyUsage = serverAuth, clientAuth"

## Convert crt to der
openssl x509 -in ca.crt -outform der -out ca.der

## Count index
touch certindex
echo 01 > certserial
echo 01 > crlnumber

## Generate CRL
openssl ca -config ca.conf -gencrl -keyfile ca.key -cert ca.crt -out root.crl.pem
openssl crl -inform PEM -in root.crl.pem -outform DER -out root.crl
rm root.crl.pem