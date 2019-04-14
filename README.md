# dldns - Dynamic LiveDNS

This program will retrieve your public IPv4 address from an
[ifconfig.co](https://ifconfig.co/) compatible service and update or
create an A record using [Gandi's LiveDNS API](https://doc.livedns.gandi.net).

Inspired by several other projects:

- [generic-dns-update](https://crates.io/crates/generic-dns-update)
- [gandi-automatic-dns](https://github.com/brianpcurran/gandi-automatic-dns)
- [gandyn](https://github.com/Chralu/gandyn)
- [jasontbradshaw/gandi-dyndns](https://github.com/jasontbradshaw/gandi-dyndns)
- [lembregtse/gandi-dyndns](https://github.com/lembregtse/gandi-dyndns)


## 👀 Usage Overview:

```
dldns [-xh] [-i ipv4 lookup] [-p json prop] [-t ttl] [-v verbosity] -s subdomain -d domain
```

## 🔍 Basic example

Create an A record for www.foo.com and set it to your IPv4 address as reported by [ipconfig.co](https://ifconfig.co/).

Using the ``-x`` option you can see what actions will be taken, but no changes will be made:

```
$ dldns -s www -d foo.com -x
A new 'A' record will be created for 'www' with an IPv4 address of 'x.x.x.x'.
Not proceeding with operation as the dry_run option was set with -x.
```

You can then apply the change by omitting the ``-x`` option:

```
$ dldns -s www -d foo.com
An 'A' record for 'www' was created with the public IPv4 address of 'x.x.x.x'.
```

If the A record already has the correct IPv4 address, no changes will be made:

```
$ dldns -s www -d foo.com
The 'A' record for 'www' is already set to the current public IPv4 address of 'x.x.x.x'.
Nothing to do.
```

## 🏞 Environment Variables

| Environment Variable Name | Example                   | Description                                | Required |
|---------------------------|---------------------------|--------------------------------------------|----------|
| GANDI_DNS_API_KEY         | kfuXhrA575aaxQKz9QRKbkJb7 | LiveDNS API Key                            | **Yes**  |
| GANDI_DNS_DOMAIN          | foo.com                   | Domain to update                           | No       |
| GANDI_DNS_SUBDOMAIN       | www                       | Subdomain to update or create the A record | No       |

`GANDI_DNS_DOMAIN` and `GANDI_DNS_SUBDOMAIN` can also be set as command line options.
The command line options take precedence over the environment variables.
By design you cannot set `GANDI_DNS_API_KEY` as a command line argument.
See the man page or the examples below for more details:


## 📦 Dependencies

- [cJSON](https://github.com/DaveGamble/cJSON) (included in repo)
- [libcurl](https://curl.haxx.se/libcurl/)

## ⚒ Building

### 😈 NetBSD
Assumes you have curl installed from [pkgsrc](https://www.pkgsrc.org/).
The ``PREFIX`` defaults to ``/usr/pkg``.

```
make
```

### 🍎 MacOS

You can use BSD Make and libcurl from [pkgsrc](https://pkgsrc.joyent.com/install-on-osx/) or use GNU Make and the base libcurl.

#### base libcurl

```
make
```

#### Pkgsrc libcurl

```
bmake PREFIX=/opt/pkg
```


### 🐧 Linux

Only tested against CentOS 7, you need to have libbsd-devel and libcurl-devel installed.

```
make
```
