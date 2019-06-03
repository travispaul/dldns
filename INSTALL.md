# dldns install instructions

You can also [build from source](https://github.com/travispaul/dldns#-building).

# Linux

```
$ sudo rpm -i https://us-east.manta.joyent.com/tpaul/public/bin/dldns-0.0.1-1.el7.x86_64.rpm
```

# MacOS

## pkgsrc install
```
# Download the dldns package 
$ curl -O https://us-east.manta.joyent.com/tpaul/public/bin/dldns-0.0.1.pkg.tgz

# by default you can only install a package signed with Joyent's key
# this option will prompt you before installing any untrusted package instead of strictly not allowing it
$ sudo sed -ie 's/always/trusted/' /opt/pkg/etc/pkg_install.conf

# Install the package
$ sudo pkg_add dldns-0.0.1.pkg.tgz

$ which dldns
/opt/pkg/bin/dldns

# You should get the same error by default:
$ dldns
EMERG,dldns.c:197,FATAL: Unable to find a value for the Gandi LiveDNS API Key in the 'GANDI_DNS_API_KEY' environment variable.

# A man page is also included
$ man dldns
```


## manual install

```
# Download the binary
$ curl -O https://us-east.manta.joyent.com/tpaul/public/bin/dldns-0.0.1.Darwin-18.6.0.tar.xz

# Extract
$ tar -xvJf dldns-0.0.1.Darwin-18.6.0.tar.xz

# You should get the same error by default:
$ ./dldns/dldns
EMERG,dldns.c:197,FATAL: Unable to find a value for the Gandi LiveDNS API Key in the 'GANDI_DNS_API_KEY' environment variable.

# Make sure the executable is in your path
$ sudo cp dldns/dldns /somewhere/in/your/path
```