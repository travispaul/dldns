# Dynamic LiveDNS 

This program will retrieve your public IPv4 address from an
[ifconfig.co](https://ifconfig.co/) compatible service and update or
create an A record using [Gandi's LiveDNS API](https://doc.livedns.gandi.net).

Inspired by several other projects:

- [generic-dns-update](https://crates.io/crates/generic-dns-update)
- [gandi-automatic-dns](https://github.com/brianpcurran/gandi-automatic-dns)
- [gandyn](https://github.com/Chralu/gandyn)
- [jasontbradshaw/gandi-dyndns](https://github.com/jasontbradshaw/gandi-dyndns)
- [lembregtse/gandi-dyndns](https://github.com/lembregtse/gandi-dyndns)


## Usage Overview:

```
dldns [-xh] [-i ipv4 lookup] [-p json prop] [-t ttl] [-v verbosity] -s subdomain -d domain
```

## Basic example

Create an A record for www.foo.com and set it to your IPv4 address as reported by [ipconfig.co](https://ifconfig.co/):

```
dldns -s www -d foo.com
```

## Environment Variables

| Environment Variable Name | Example                   | Description                                | Required |
|---------------------------|---------------------------|--------------------------------------------|----------|
| GANDI_DNS_API_KEY         | kfuXhrA575aaxQKz9QRKbkJb7 | LiveDNS API Key                            | Yes      |
| GANDI_DNS_DOMAIN          | foo.com                   | Domain to update                           | No       |
| GANDI_DNS_SUBDOMAIN       | www                       | Subdomain to update or create the A record | No       |

`GANDI_DNS_DOMAIN` and `GANDI_DNS_SUBDOMAIN` can also be set as command line options.
The command line options take precedence over the environment variables.
By design you cannot set `GANDI_DNS_API_KEY` as a command line argument.
See the man page or the examples below for more details:


## Dependencies

- [cJSON](https://github.com/DaveGamble/cJSON)
- [libcurl](https://curl.haxx.se/libcurl/)
