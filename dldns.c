/*
  BSD 2-Clause License

  Copyright (c) 2019, Travis Paul
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#ifdef __linux__
#include <bsd/string.h>
#include <bsd/stdlib.h>
#endif

#include "cJSON.h"
#include "req.h"
#include "log.h"

#define CREATE 0
#define UPDATE 1
#define ACCURATE 2

#define LIVEDNS_MAX_TTL 2592000
#define LIVEDNS_MIN_TTL 300
#define TTL_CHAR_BUFSIZE 8

#define TRACE 0
#define DEBUG 1
#define INFO 2
#define WARN 3
#define ERROR 4
#define FATAL 5

#define IPV4_LOOKUP_URL_DEFAULT "https://ifconfig.co/json"
#define IPV4_LOOKUP_PROPERTY_DEFAULT "ip"

static void usage(void);

static int verbosity;

int
main(int argc, char * argv[])
{
	extern char * optarg;
	extern int optind;
	int opt_char;
	size_t optarg_length;

	req_options * options;
	const char * headers[2];
	const char * new_ip_array[1];
	char url[2048]; /* XXX use malloc */
	char api_key_header[256];
	char current_ipv4[16];

	char * api_key;
	char * domain;
	char * subdomain;
	char * ipv4_lookup_url;
	char * ipv4_lookup_property;

	cJSON * root, * item, * type, * name, * values, * ip, * new_obj, * new_array;

	int ttl = LIVEDNS_MIN_TTL;
	char ttl_buffer[TTL_CHAR_BUFSIZE + 1];
	unsigned short dry_run;
	unsigned short skip_GET;
	unsigned short update_mode;
	long last_status;
	char last_status_buffer[4];

	domain = NULL;
	subdomain = NULL;
	ipv4_lookup_url = NULL;
	ipv4_lookup_property = NULL;
	update_mode = CREATE;
	dry_run = 0;

	verbosity = INFO;

	log_set_level(verbosity);

	setprogname(argv[0]);

	while ((opt_char = getopt(argc, argv, "d:i:f:p:s:t:v:x")) != -1) {
		switch (opt_char) {

			/* domain */
			case 'd':
				optarg_length = strlen(optarg);
				domain = malloc(optarg_length + 1);
				if (domain == NULL) {
					log_fatal("malloc failed at %s:%d", __FILE__, __LINE__);
					exit(EXIT_FAILURE);
				}
				strlcpy(domain, optarg, optarg_length + 1);
				break;

			/* ipv4 lookup provider URL */
			case 'i':
				optarg_length = strlen(optarg);
				ipv4_lookup_url = malloc(optarg_length + 1);
				if (ipv4_lookup_url == NULL) {
					log_fatal("malloc failed at %s:%d", __FILE__, __LINE__);
					exit(EXIT_FAILURE);
				}
				strlcpy(ipv4_lookup_url, optarg, optarg_length + 1);
				break;

			/* (force) Use the provided IPv4 address*/
			case 'f':
				snprintf(current_ipv4, sizeof current_ipv4, "%s", optarg);
				skip_GET = 1;
				break;

			/* JSON property containing ipv4 address */
			case 'p':
				optarg_length = strlen(optarg);
				ipv4_lookup_property = malloc(optarg_length + 1);
				if (ipv4_lookup_property == NULL) {
					log_fatal("malloc failed at %s:%d", __FILE__, __LINE__);
					exit(EXIT_FAILURE);
				}
				strlcpy(ipv4_lookup_property, optarg, optarg_length + 1);
				break;

			/* subdomain */
			case 's':
				optarg_length = strlen(optarg);
				subdomain = malloc(optarg_length + 1);
				if (subdomain == NULL) {
					log_fatal("malloc failed at %s:%d", __FILE__, __LINE__);
					exit(EXIT_FAILURE);
				}
				strlcpy(subdomain, optarg, optarg_length + 1);
				break;

			/* TTL for A record */
			case 't':
				ttl = atoi(optarg);
				break;

			/* verbosity */
			case 'v':
				verbosity = atoi(optarg);
				if (verbosity <= TRACE) {
					verbosity = TRACE;
				}
				if (verbosity >= FATAL) {
					verbosity = FATAL;
				}
				log_set_level(verbosity);
				break;

			/* dry run */
			case 'x':
				dry_run = 1;
				break;

			case '?':
			case 'h':
			default:
				usage();
				/* NOTREACHED */
		}
	}

	argc -= optind;
	argv += optind;

	api_key = getenv("GANDI_DNS_API_KEY");
	if (api_key == NULL) {
		log_fatal("Unable to find a value for the Gandi LiveDNS API Key in "
			"the 'GANDI_DNS_API_KEY' environment variable.");
		exit(EXIT_FAILURE);
	}

	if (domain == NULL) {
		domain = getenv("GANDI_DNS_DOMAIN");
		if (domain == NULL || strlen(domain) < 3) {
			log_fatal("Unable to find a value for 'domain' in either the -d "
				"argument or the 'GANDI_DNS_DOMAIN' environment variable.");
			exit(EXIT_FAILURE);
		}
	}

	log_debug("domain = %s", domain);

	if (subdomain == NULL) {
		subdomain = getenv("GANDI_DNS_SUBDOMAIN");
		if (subdomain == NULL || strlen(subdomain) < 1) {
			log_fatal("Unable to find a value for 'subdomain' in either the -s "
				"argument or the 'GANDI_DNS_SUBDOMAIN' environment variable.");
			exit(EXIT_FAILURE);
		}
	}

	log_debug("subdomain = %s", subdomain);

	if (ipv4_lookup_url == NULL) {
		ipv4_lookup_url = IPV4_LOOKUP_URL_DEFAULT;
	}

	log_debug("ipv4_lookup_url = %s", ipv4_lookup_url);

	if (ipv4_lookup_property == NULL) {
		ipv4_lookup_property = IPV4_LOOKUP_PROPERTY_DEFAULT;
	}

	log_debug("ipv4_lookup_property = %s", ipv4_lookup_property);

	snprintf(ttl_buffer, TTL_CHAR_BUFSIZE, "%d", ttl);

	if (ttl > LIVEDNS_MAX_TTL) {
		ttl = LIVEDNS_MAX_TTL;
		snprintf(ttl_buffer, TTL_CHAR_BUFSIZE, "%d", ttl);
		log_warn("Desired ttl exceeded LIVEDNS_MAX_TTL, capped to %s",
			ttl_buffer);
	}

	if (ttl < LIVEDNS_MIN_TTL) {
		ttl = LIVEDNS_MIN_TTL;
		snprintf(ttl_buffer, TTL_CHAR_BUFSIZE, "%d", ttl);
		log_warn("Desired ttl lower than LIVEDNS_MIN_TTL, increased to %s",
			ttl_buffer);
	}

	snprintf(ttl_buffer, TTL_CHAR_BUFSIZE, "%d", ttl);
	log_debug("ttl = %s", ttl_buffer);

	if (!skip_GET) {
		root = req_get(ipv4_lookup_url, NULL, &last_status);

		if (root == NULL) {
			log_fatal("failed to fetch IPv4 address, no parsable JSON "
				"response returned");
			exit(EXIT_FAILURE);
		}

		snprintf(last_status_buffer, 4, "%ld", last_status);

		log_debug("HTTP status from ipv4_lookup_url = %s", last_status_buffer);

		log_trace("response from ipv4_lookup_url = %s",
			cJSON_PrintUnformatted(root));

		if (last_status >= 400) {
			log_fatal("received error response from ipv4_lookup_url = %s",
				last_status_buffer);
			exit(EXIT_FAILURE);
		}

		if (cJSON_HasObjectItem(root, ipv4_lookup_property)) {
			ip = cJSON_GetObjectItem(root, ipv4_lookup_property);
			snprintf(current_ipv4, sizeof current_ipv4, "%s",
				cJSON_GetStringValue(ip));
			cJSON_free(ip);
			log_debug("current_ipv4 = %s", current_ipv4);
		}
		cJSON_free(root);
	}

	/* XXX Use malloc here */
	snprintf(url, sizeof url,
		"https://dns.api.gandi.net/api/v5/domains/%s/records",
		domain);

	log_debug("url = %s", url);

	/* XXX Use malloc here */
	snprintf(api_key_header, sizeof api_key_header, "X-Api-Key: %s", api_key);

	options = malloc(sizeof(req_options));
	headers[0] = api_key_header;
	headers[1] = NULL;
	options->headers = headers;

	root = req_get(url, options, &last_status);

	if (root == NULL) {
		log_fatal("failed to fetch DNS records, no parsable JSON response "
			"returned from LiveDNS");
		exit(EXIT_FAILURE);
	}

	snprintf(last_status_buffer, 4, "%ld", last_status);

	log_debug("HTTP status from LiveDNS GET = %s", last_status_buffer);

	log_trace("response from LiveDNS GET= %s", cJSON_PrintUnformatted(root));

	if (last_status >= 400) {
		log_error("received an error response from LiveDNS GET = %s",
			last_status_buffer);

		item = cJSON_GetObjectItem(root, "message");

		if (item == NULL) {
			log_fatal("No error message provided");
			exit(EXIT_FAILURE);
		}

		log_fatal("error = %s", cJSON_GetStringValue(item));
		exit(EXIT_FAILURE);
	}

	cJSON_ArrayForEach(item, root) {
		type = cJSON_GetObjectItem(item, "rrset_type");
		name = cJSON_GetObjectItem(item, "rrset_name");
		values = cJSON_GetObjectItem(item, "rrset_values");

		if (strcmp(cJSON_GetStringValue(type), "A") == 0 &&
			strcmp(cJSON_GetStringValue(name), subdomain) == 0) {

			log_debug("found matching record = %s",
				cJSON_PrintUnformatted(item));

			update_mode = UPDATE;

			cJSON_ArrayForEach(ip, values) {
				if (strcmp(ip->valuestring, current_ipv4) == 0) {
					update_mode = ACCURATE;
				} else {
					log_info("Found stale IPv4 address in A record. "
						"Stale value = %s", ip->valuestring);
				}
			}
		}
		cJSON_free(type);
		cJSON_free(name);
		cJSON_free(values);
	}
	cJSON_free(root);

	switch (update_mode) {
		case ACCURATE:
			log_info("The 'A' record for '%s' is already set to the current "
				"public IPv4 address of '%s'. Nothing to do.",
				subdomain, current_ipv4);
		break;

		case UPDATE:
			log_info("'A' record needs to be updated for '%s'", subdomain);
			if (dry_run) {
				log_info("Not proceeding with operation as dry_run was set "
					"with -x");
				log_info("The 'A' record for '%s' will be updated with an IPv4 "
					"address of '%s'. Not proceeding with operation as the "
					"dry_run option was set with -x.",
					subdomain, current_ipv4);
				exit(EXIT_SUCCESS);
			}

			new_ip_array[0] = current_ipv4;
			new_obj = cJSON_CreateObject();
			new_array = cJSON_CreateStringArray(new_ip_array, 1);

			cJSON_AddItemToObject(new_obj, "rrset_values", new_array);
			cJSON_AddNumberToObject(new_obj, "rrset_ttl", ttl);

			snprintf(url, sizeof url,
				"https://dns.api.gandi.net/api/v5/domains/%s/records/%s/A",
				domain, subdomain);

			root = req_put(url, new_obj, options, &last_status);

			if (root == NULL) {
				log_fatal("failed to update DNS record, no parsable "
					"JSON response returned from LiveDNS");
				exit(EXIT_FAILURE);
			}

			snprintf(last_status_buffer, 4, "%ld", last_status);

			log_debug("HTTP status from LiveDNS PUT = %s", last_status_buffer);

			log_trace("response from LiveDNS PUT = %s",
				cJSON_PrintUnformatted(root));

			if (last_status >= 200 && last_status <= 299) {
				log_info("The 'A' record for '%s' was updated to the public "
					"IPv4 address of '%s'", subdomain, current_ipv4);
			} else {
				log_error("The 'A' record for '%s' was NOT updated to the "
					"public IPv4 address of '%s'. Set increased verbosity to "
					"see details and try again.", subdomain, current_ipv4);
			}

			cJSON_free(new_array);
			cJSON_free(new_obj);
		break;

		case CREATE:
			log_info("'A' record needs to be created = %s", subdomain);

			if (dry_run) {
				log_info("Not proceeding with operation as dry_run was set "
					"with -x");
				log_info("A new 'A' record will be created for '%s' with an IPv4 "
					"address of '%s'. Not proceeding with operation as the "
					"dry_run option was set with -x.",
					subdomain, current_ipv4);
				exit(EXIT_SUCCESS);
			}

			new_ip_array[0] = current_ipv4;
			new_obj = cJSON_CreateObject();
			new_array = cJSON_CreateStringArray(new_ip_array, 1);

			cJSON_AddItemToObject(new_obj, "rrset_values", new_array);
			cJSON_AddStringToObject(new_obj, "rrset_name", subdomain);
			cJSON_AddStringToObject(new_obj, "rrset_type", "A");
			cJSON_AddNumberToObject(new_obj, "rrset_ttl", ttl);

			log_trace("JSON to be used for record creation = %s",
					cJSON_PrintUnformatted(new_obj));

			root = req_post(url, new_obj, options, &last_status);

			if (root == NULL) {
				log_fatal("failed to create DNS record, no parsable "
					"JSON response returned from LiveDNS");
				exit(EXIT_FAILURE);
			}

			snprintf(last_status_buffer, 4, "%ld", last_status);

			log_debug("HTTP status from LiveDNS POST = %s", last_status_buffer);

			log_trace("response from LiveDNS POST = %s",
				cJSON_PrintUnformatted(root));

			if (last_status >= 200 && last_status <= 299) {
				log_info("An 'A' record for '%s' was created with the public "
					"IPv4 address of '%s'", subdomain, current_ipv4);
			} else {
				log_error("An 'A' record for '%s' was NOT created with the "
					"public IPv4 address of '%s'", subdomain, current_ipv4);
			}

			cJSON_free(new_array);
			cJSON_free(new_obj);
		break;
	}
	return 0;
}

static void
usage(void)
{
	fprintf(stderr, "Usage:\n  %s [-xh] [-i ipv4 lookup] [-p json prop] "
		"[-t ttl] [-v verbosity] -s subdomain -d domain\n", getprogname());
	exit(EXIT_FAILURE);
}
