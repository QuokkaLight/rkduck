# rkduck - Rootkit for Linux v4 [![Build Status](https://travis-ci.org/QuokkaLight/rkduck.svg?branch=master)](https://travis-ci.org/QuokkaLight/rkduck)

**rkduck** is a Loadable Kernel Module rootkit for the latest Linux Kernels v4. This is still a work in progress.

## Features

 * **Stealth**
	 * Hide files, directories, processes
 * **Communication**
	 * SSH
	 * Direct shell (unencrypted)
	 * Reverse shell (unencrypted) 
 * **Keylogger**
	 * Recording of the keystrokes of every user.
	 * Information sent periodically
 * **Crumbs**
	 * A user space CLI program allowing the user to control the rootkit configuration during its execution
	 * Requires an authentication to be used (hardcoded key stored in rduck, the configuration section has more information about it)

## Tests

At the moment we didn't get the chance to test our rootkit on different versions of Linux to make sure everything is working as intended. If you want to report a bug feel free to create an issue or send us an email at <quokkalight@gmail.com>.

## Contributors

 * **mpgn** - [Twitter](http://twitter.com/mpgn_x64)
 * **RainbowLyte** - [Twitter](http://twitter.com/Rainbowlyte)

                      _.._
                     /   a\__,
                     \  -.___/
                      \  \
                 (\____)  \
             |\_(         ))
        _____|   (_        /________
             _\____(______/__
                  ______
                 