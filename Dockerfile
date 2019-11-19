FROM openjdk:8

COPY . /opt/canu/
RUN apt-get -qqy update --fix-missing && \
    apt-get -qqy dist-upgrade && \
    apt-get -qqy install --no-install-recommends \
                 apt-transport-https \
                 build-essential \
                 ca-certificates \
                 curl \
                 git \
                 tree \
                 wget && \
    cd /opt/canu/src && \
    make && \
    echo "deb [signed-by=/usr/share/keyrings/cloud.google.gpg] https://packages.cloud.google.com/apt cloud-sdk main" | tee -a /etc/apt/sources.list.d/google-cloud-sdk.list && \
    curl https://packages.cloud.google.com/apt/doc/apt-key.gpg | apt-key --keyring /usr/share/keyrings/cloud.google.gpg add - && \
    apt-get -qqy update && apt-get -qqy install google-cloud-sdk && \
    gcloud config set core/disable_usage_reporting true && \
    gcloud config set component_manager/disable_update_check true && \
    gcloud config set metrics/environment github_docker_image && \
    apt-get -qqy clean && \
    rm -rf /tmp/* \
           /var/tmp/* \
           /var/cache/apt/* \
           /var/lib/apt/lists/* \
           /usr/share/man/?? \
           /usr/share/man/??_*

ENV PATH="/opt/canu/Linux-amd64/bin:${PATH}"
RUN canu --version && \
    echo -e "\033[33mPlease watch out for the version of canu\033[0m"
