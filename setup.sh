#!/bin/bash

: <<'DISCLAIMER'

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

This script is licensed under the terms of the MIT license.
Unless otherwise noted, code reproduced herein
was written for this script.

- The Pimoroni Crew -

DISCLAIMER

# script control variables

productname="Speaker pHAT" # the name of the product to install
scriptname="speakerphat-setup.sh" # the name of this script
forcesudo="no" # whether the script requires to be ran with root privileges
promptreboot="no" # whether the script should always prompt user to reboot

FORCE=$1
ASK_TO_REBOOT=false
CURRENT_SETTING=false
MIN_INSTALL=false
FAILED_PKG=false
REMOVE_PKG=false
UPDATE_DB=false

AUTOSTART=~/.config/lxsession/LXDE-pi/autostart
BOOTCMD=/boot/cmdline.txt
CONFIG=/boot/config.txt
APTSRC=/etc/apt/sources.list
INITABCONF=/etc/inittab
BLACKLIST=/etc/modprobe.d/raspi-blacklist.conf
LOADMOD=/etc/modules

# function define

confirm() {
    if [ "$FORCE" == '-y' ]; then
        true
    else
        read -r -p "$1 [y/N] " response < /dev/tty
        if [[ $response =~ ^(yes|y|Y)$ ]]; then
            true
        else
            false
        fi
    fi
}

prompt() {
        read -r -p "$1 [y/N] " response < /dev/tty
        if [[ $response =~ ^(yes|y|Y)$ ]]; then
            true
        else
            false
        fi
}

success() {
    echo -e "$(tput setaf 2)$1$(tput sgr0)"
}

inform() {
    echo -e "$(tput setaf 6)$1$(tput sgr0)"
}

warning() {
    echo -e "$(tput setaf 1)$1$(tput sgr0)"
}

newline() {
    echo ""
}

progress() {
    count=0
    until [ $count -eq $1 ]; do
        echo -n "..." && sleep 1
        ((count++))
    done
    echo
}
sudocheck() {
    if [ $(id -u) -ne 0 ]; then
        echo -e "Install must be run as root. Try 'sudo ./$scriptname'\n"
        exit 1
    fi
}

sysclean() {
    sudo apt-get clean && sudo apt-get autoclean
    sudo apt-get -y autoremove &> /dev/null
}

sysupdate() {
    if ! $UPDATE_DB; then
        echo "Updating apt indexes..." && progress 3 &
        sudo apt-get update 1> /dev/null || { warning "Apt failed to update indexes!" && exit 1; }
        echo "Reading package lists..."
        progress 3 && UPDATE_DB=true
    fi
}

sysupgrade() {
    sudo apt-get upgrade
    sudo apt-get clean && sudo apt-get autoclean
    sudo apt-get -y autoremove &> /dev/null
}

sysreboot() {
    warning "Some changes made to your system require"
    warning "your computer to reboot to take effect."
    newline
    if prompt "Would you like to reboot now?"; then
        sync && sudo reboot
    fi
}

apt_pkg_req() {
    APT_CHK=$(dpkg-query -W -f='${Status}\n' "$1" 2> /dev/null | grep "install ok installed")

    if [ "" == "$APT_CHK" ]; then
        echo "$1 is required"
        true
    else
        echo "$1 is already installed"
        false
    fi
}

apt_pkg_install() {
    echo "Installing $1..."
    sudo apt-get --yes install "$1" 1> /dev/null || { echo -e "Apt failed to install $1!\nFalling back on pypi..." && return 1; }
    echo
}

: <<'MAINSTART'

Perform all global variables declarations as well as function definition
above this section for clarity, thanks!

MAINSTART

# checks and init

if [ $forcesudo == "yes" ]; then
    sudocheck
fi

newline
echo "This script will install everything needed to use"
echo "$productname"
newline
warning "--- Warning ---"
newline
echo "Always be careful when running scripts and commands"
echo "copied from the internet. Ensure they are from a"
echo "trusted source."
newline

echo "Checking for required packages..."

if apt_pkg_req "wiringpi" &> /dev/null; then
    sysupdate && apt_pkg_install "wiringpi"
fi

echo "Enabling I2C interface..."

sudo raspi-config nonint do_i2c 0
sleep 1 && echo

echo "Installing ALSA plugin..."

sudo cp ./dependencies/usr/local/lib/libphatmeter.so.0.0.0 /usr/local/lib/
ln -s /usr/local/lib/libphatmeter.so.0.0.0 /usr/local/lib/libphatmeter.so.0 &> /dev/null
ln -s /usr/local/lib/libphatmeter.so.0.0.0 /usr/local/lib/libphatmeter.so &> /dev/null
pulseaudio -k &> /dev/null # kill pulseaudio daemon if running
if [ -e $AUTOSTART ] && ! grep -q "@pulseaudio -k" $AUTOSTART; then
    echo "@pulseaudio -k" >> $AUTOSTART
fi # kills pulseaudio on boot

echo -e "\nConfiguring sound output"

if [ -e $CONFIG ] && grep -q "^device_tree=$" $CONFIG; then
    DEVICE_TREE=false
fi

if $DEVICE_TREE; then
    echo -e "\nAdding Device Tree Entry to $CONFIG"

    if [ -e $CONFIG ] && grep -q "^dtoverlay=i2s-mmap$" $CONFIG; then
        echo "i2s-mmap overlay already active"
    else
        echo "dtoverlay=i2s-mmap" | sudo tee -a $CONFIG
        ASK_TO_REBOOT=true
    fi

    if [ -e $CONFIG ] && grep -q "^dtoverlay=hifiberry-dac$" $CONFIG; then
        echo "DAC overlay already active"
    else
        echo "dtoverlay=hifiberry-dac" | sudo tee -a $CONFIG
        ASK_TO_REBOOT=true
    fi

    if [ -e $BLACKLIST ]; then
        echo -e "\nCommenting out Blacklist entry in\n$BLACKLIST"
        sudo sed -i -e "s|^blacklist[[:space:]]*i2c-bcm2708.*|#blacklist i2c-bcm2708|" \
                    -e "s|^blacklist[[:space:]]*snd-soc-pcm512x.*|#blacklist snd-soc-pcm512x|" \
                    -e "s|^blacklist[[:space:]]*snd-soc-wm8804.*|#blacklist snd-soc-wm8804|" $BLACKLIST &> /dev/null
    fi
else
    echo -e "\nNo Device Tree Detected, not supported\n" && exit 1
fi

if [ -e $CONFIG ] && grep -q -E "^dtparam=audio=on$" $CONFIG; then
    bcm2835off="no"
    echo -e "\nDisabling default sound driver"
    sudo sed -i "s|^dtparam=audio=on$|#dtparam=audio=on|" $CONFIG &> /dev/null
    if [ -e $LOADMOD ] && grep -q "^snd-bcm2835" $LOADMOD; then
        sudo sed -i "s|^snd-bcm2835|#snd-bcm2835|" $LOADMOD &> /dev/null
    fi
    ASK_TO_REBOOT=true
elif [ -e $LOADMOD ] && grep -q "^snd-bcm2835" $LOADMOD; then
    bcm2835off="no"
    echo -e "\nDisabling default sound module"
    sudo sed -i "s|^snd-bcm2835|#snd-bcm2835|" $LOADMOD &> /dev/null
    ASK_TO_REBOOT=true
else
    echo -e "\nDefault sound driver currently not loaded"
    bcm2835off="yes"
fi

if [ -e /etc/asound.conf ]; then
    if [ -e /etc/asound.conf.backup ]; then
        sudo rm -f /etc/asound.conf.backup
    fi
    sudo mv /etc/asound.conf /etc/asound.conf.backup
fi
sudo cp ./dependencies/etc/asound.conf /etc/asound.conf

newline && success "All done!" && newline
echo "Enjoy your new $productname!" && newline

if [ $promptreboot == "yes" ] || $ASK_TO_REBOOT; then
    sysreboot
fi

exit 0
