FROM centos:centos7

WORKDIR /home

# ENV HTTP_PROXY="http://10.214.224.168:7890"
# ENV HTTPS_PROXY="http://10.214.224.168:7890"
# ENV https_proxy="http://10.214.224.168:7890"
# ENV http_proxy="http://10.214.224.168:7890"

RUN cp /etc/yum.repos.d/CentOS-Base.repo /etc/yum.repos.d/CentOS-Base.repo.bak
# RUN curl -o /etc/yum.repos.d/CentOS-Base.repo http://mirrors.aliyun.com/repo/Centos-7.repo
# RUN curl -o /etc/yum.repos.d/CentOS-Base.repo http://mirrors.163.com/.help/CentOS7-Base-163.repo
# RUN curl -o /etc/yum.repos.d/CentOS-Base.repo https://mirrors.tencent.com/repo/centos7_base.repo && yum clean all && yum makecache
RUN curl -o /etc/yum.repos.d/CentOS-Base.repo https://gitee.com/fishfishfishfishfish/centos.repo/raw/master/CentOS-Base.repo && yum clean all && yum makecache
RUN yum install -y wget && yum install -y git
RUN wget https://gitee.com/fishfishfishfishfish/centos.repo/raw/master/go1.21.13.linux-amd64.tar.gz && \
    tar -C /usr/local -xzf go1.21.13.linux-amd64.tar.gz && \
    ln -s /usr/local/go/bin/go /usr/local/bin/go
RUN wget https://gitee.com/fishfishfishfishfish/centos.repo/raw/master/cmake-3.12.4-Linux-x86_64.tar.gz && \
    tar -C /usr/local -zxvf cmake-3.12.4-Linux-x86_64.tar.gz && \
    ln -s /usr/local/cmake-3.12.4-Linux-x86_64/bin/cmake /usr/local/bin/cmake
RUN yum install -y scl-utils && yum install -y centos-release-scl
RUN curl -o /etc/yum.repos.d/CentOS-SCLo-scl.repo https://gitee.com/fishfishfishfishfish/centos.repo/raw/master/CentOS-SCLo-sclo.repo && \
    curl -o /etc/yum.repos.d/CentOS-SCLo-scl-rh.repo https://gitee.com/fishfishfishfishfish/centos.repo/raw/master/CentOS-SCLo-rh.repo && \
    yum clean all && yum makecache
RUN yum install -y wget
RUN yum install -y devtoolset-11-toolchain
RUN scl enable devtoolset-11 bash
RUN wget https://gitee.com/fishfishfishfishfish/centos.repo/raw/master/perl-5.40.0.tar.gz && tar -zxvf perl-5.40.0.tar.gz && cd perl-5.40.0 && ./Configure -des -Dprefix=/home/localperl && make && make test && make install
# RUN wget https://www.cpan.org/src/5.0/perl-5.22.3.tar.gz && tar -zxvf perl-5.22.3.tar.gz && cd perl-5.22.3 && ./Configure -des -Dprefix=/home/localperl && make && make test && make install
RUN wget https://gitee.com/fishfishfishfishfish/centos.repo/raw/master/openssl-1.1.1q.tar.gz && tar -zxvf openssl-1.1.1q.tar.gz && \
    cd openssl-1.1.1q && ./config --prefix=/usr/local/openssl && ./config -t make && make install
# RUN wget https://github.com/openssl/openssl/releases/download/OpenSSL_1_1_1g/openssl-1.1.1g.tar.gz && tar -zxvf openssl-1.1.1g.tar.gz
# RUN wget https://github.com/openssl/openssl/releases/download/OpenSSL_1_1_0/openssl-1.1.0.tar.gz && tar -zxvf openssl-1.1.0.tar.gz
RUN ldd /usr/local/openssl/bin/openssl && \
    ln -s /usr/local/openssl/bin/openssl /usr/bin/openssl && \
    ln -s /usr/local/openssl/include/openssl /usr/include/openssl && \
    ln -s /usr/local/openssl/lib/libssl.so.1.1 /usr/lib64/libssl.so.1.1 && \
    ln -s /usr/local/openssl/lib/libcrypto.so.1.1 /usr/lib64/libcrypto.so.1.1
RUN export PATH=/usr/local/openssl:$PATH