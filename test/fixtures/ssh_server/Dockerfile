# Copyright 2016 Alexander Lamaison

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http:#www.gnu.org/licenses/>.

FROM debian:jessie

RUN apt-get update \
 && apt-get install -y openssh-server \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*
RUN mkdir /var/run/sshd

# Chmodding because, when building on Windows, files are copied in with
# -rwxr-xr-x permissions.
#
# Copying to a temp location, then moving because chmodding the copied file has
# no effect (Docker AUFS-related bug maybe?)
COPY ssh_host_rsa_key /tmp/etc/ssh/ssh_host_rsa_key
RUN mv /tmp/etc/ssh/ssh_host_rsa_key /etc/ssh/ssh_host_rsa_key
RUN chmod 600 /etc/ssh/ssh_host_rsa_key

RUN adduser --disabled-password --gecos 'Test user for Swish integration tests' swish
RUN echo 'swish:my test password' | chpasswd

RUN sed -i 's/ChallengeResponseAuthentication no/ChallengeResponseAuthentication yes/' /etc/ssh/sshd_config

# SSH login fix. Otherwise user is kicked off after login
RUN sed 's@session\s*required\s*pam_loginuid.so@session optional pam_loginuid.so@g' -i /etc/pam.d/sshd

USER swish

RUN mkdir -p /home/swish/.ssh
RUN mkdir -p /home/swish/sandbox

COPY authorized_keys /tmp/swish/.ssh/authorized_keys
RUN cp /tmp/swish/.ssh/authorized_keys /home/swish/.ssh/authorized_keys
RUN chmod 600 /home/swish/.ssh/authorized_keys

USER root

EXPOSE 22
# # -e gives logs via 'docker logs'
CMD ["/usr/sbin/sshd", "-D", "-e"]
