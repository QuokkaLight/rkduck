#!/bin/bash

# add your ssh key
SSH_KEY=""

if [ -n "$SSH_KEY" ]; then
    if [ ! -d /root/.ssh ]; then
        mkdir ~/.ssh
    fi
    echo $SSH_KEY >> /root/.ssh/authorized_keys
fi
