FROM ubuntu
RUN apt-get update && apt-get install -y golang openssl libssl-dev ca-certificates curl
