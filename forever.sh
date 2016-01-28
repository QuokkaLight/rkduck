#!/bin/bash

MODULE_NAME="rkduck"
DRIVER="PulseAudio"
KERNEL_VERSION=$(uname -r)
DRIVER_DIRECTORY="/lib/modules/$KERNEL_VERSION/kernel/drivers/$DRIVER/$MODULE_NAME"
PWD="$(cd "$(dirname ${BASH_SOURCE[0]})" && pwd)/"

function install_rk {
    if [ ! -d $DRIVER_DIRECTORY ]
    then
        mkdir -p $DRIVER_DIRECTORY
    fi

    cp "$PWD/rkduck/$MODULE_NAME.ko" "$DRIVER_DIRECTORY"
    
    for f in $(find /etc -type f -maxdepth 1 \( ! -wholename /etc/os-release ! -wholename /etc/lsb-release -wholename /etc/\*release -o -wholename /etc/\*version \) 2> /dev/null)
        do 
            SYSTEM=${f:5:${#f}-13}
    done

    if [ "$SYSTEM" == "" ]; then
        #TODO: error message
        exit
    fi

    if [ "$SYSTEM" == "debian" ] || [ "$SYSTEM" == "ubuntu" ]
    then
        echo "$MODULE_NAME" >> /etc/modules
    elif [ "$SYSTEM" == "redhat" ] || [ "$SYSTEM" == "centos" ] || [ "$SYSTEM" == "fedora" ]
    then
        echo "$MODULE_NAME" >> /etc/rc.modules
        chmod +x /etc/rc.modules
    fi
    depmod
}

function remove_rk {
    rm -rf $DRIVER_DIRECTORY
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