# Lab 0: Networking Warmup


## Preliminaries
> Fill in your name and email address.

J.K. Xu <2507550027@qq.com>

> preliminary comments on your submission

1. Install Ubuntu 18.04.1 live server on your virtual box : [Ubuntu 18.04.1 live server iso](https://old-releases.ubuntu.com/releases/18.04.1/ubuntu-18.04.1-live-server-amd64.iso)

2. run this setup script 

**Do not just copy and paste the all command below! At least you need to replace your username at the end.**

```sh
#!/bin/sh

if [ -z "$SUDO_USER" ]; then
    # if the user didn't call us with sudo, re-execute
    exec sudo $0 "$@"
fi

### update sources and get add-apt-repository
apt-get update
apt-get -y install software-properties-common

### add the extended source repos
add-apt-repository multiverse
add-apt-repository universe
add-apt-repository restricted

### make sure we're totally up-to-date now
apt-get update
apt-get -y dist-upgrade

### install the software we need for the VM and build env
apt-get -y install build-essential gcc gcc-8 g++ g++-8 cmake libpcap-dev htop jnettop screen   \
                   emacs-nox vim-nox automake pkg-config libtool libtool-bin git tig links     \
                   parallel iptables mahimahi mininet net-tools tcpdump wireshark telnet socat \
                   clang clang-format clang-tidy clang-tools coreutils bash doxygen graphviz   \
                   virtualbox-guest-utils netcat-openbsd

## make a sane set of alternatives for gcc, and make gcc-8 the default
# GCC
update-alternatives --remove-all gcc &>/dev/null
for ver in 7 8; do
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-${ver} $((10 * ${ver})) \
        $(for prog in g++ gcc-ar gcc-nm gcc-ranlib gcov gcov-dump gcov-tool; do
            echo "--slave /usr/bin/${prog} ${prog} /usr/bin/${prog}-${ver}"
        done)
done

### add user to the virtualbox shared folder group and enable the virtualbox guest utils
adduser "$SUDO_USER" <replace with your username>
systemctl enable virtualbox-guest-utils.service
``` 

3. Complete tutorial for environment setup: [enviroment setup](./attachments/stanford_edu_class_cs144_vm_howto_vm_howto_iso_html.pdf)




> Please cite any offline or online sources you consulted while preparing your submission, other than the lab handouts, course text, lecture notes

Nothing



## Writing webget

#### Data Structures
> Copy here the declaration of each new or changed `struct` or `struct member`, `global` or `static variable`, `typedef`, or `enumeration`.
> Identify the purpose of each in 25 words or less.

Nothing


#### Algorithm

> Briefly describe your implementation of `get_URL`, and what are some subtle points to consider when constructing http header?

get_URL use TCPSocket to connect a given host with http protocol, then construct a simple http request and send it with sock.write()  

Each http line's end delimiter is `\r\n` rather than `\n`.





