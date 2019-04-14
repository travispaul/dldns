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
#endif

#include "cJSON.h"
#include "req.h"

#define CREATE 0
#define UPDATE 1
#define ACCURATE 2

#define LIVEDNS_MAX_TTL 2592000
#define LIVEDNS_MIN_TTL 300
#define TTL_CHAR_BUFSIZE 8

#define EMERG 0
#define ALERT 1
#define CRIT 2
#define ERR 3
#define WARN 4
#define NOTICE 5
#define INFO 6
#define DEBUG 7

#define IPV4_LOOKUP_URL_DEFAULT "https://ifconfig.co/json"
#define IPV4_LOOKUP_PROPERTY_DEFAULT "ip"

static void
usage(void);

static void
fail_hard_if_null(void *, const char *, const char *, unsigned int);

static void
logmsg(int, const char *, const char *, const char *, unsigned int);

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
	unsigned short update_mode;
	long last_status;
	char last_status_buffer[4];

	domain = NULL;
	subdomain = NULL;
	ipv4_lookup_url = NULL;
	ipv4_lookup_property = NULL;
	update_mode = CREATE;
	dry_run = 0;

	verbosity = ERR;

	while ((opt_char = getopt(argc, argv, "d:i:p:s:t:v:x")) != -1) {
		switch (opt_char) {

			/* domain */
			case 'd':
				optarg_length = strlen(optarg);
				domain = malloc(optarg_length + 1);
				fail_hard_if_null(domain, NULL, __FILE__, __LINE__);
				strlcpy(domain, optarg, optarg_length + 1);
				break;

			/* ipv4 lookup provider URL */
			case 'i':
				optarg_length = strlen(optarg);
				ipv4_lookup_url = malloc(optarg_length + 1);
				fail_hard_if_null(ipv4_lookup_url, NULL,
					__FILE__, __LINE__);
				strlcpy(ipv4_lookup_url, optarg,
					optarg_length + 1);
				break;

			/* JSON property containing ipv4 address */
			case 'p':
				optarg_length = strlen(optarg);
				ipv4_lookup_property = malloc(optarg_length + 1);
				fail_hard_if_null(ipv4_lookup_property, NULL,
					__FILE__, __LINE__);
				strlcpy(ipv4_lookup_property, optarg,
					optarg_length + 1);
				break;

			/* subdomain */
			case 's':
				optarg_length = strlen(optarg);
				subdomain = malloc(optarg_length + 1);
				fail_hard_if_null(subdomain, NULL,
					__FILE__, __LINE__);
				strlcpy(subdomain, optarg, optarg_length + 1);
				break;

			/* TTL for A record */
			case 't':
				ttl = atoi(optarg);
				break;

			/* verbosity */
			case 'v':
				verbosity = atoi(optarg);
				if (verbosity <= EMERG) {
					verbosity = EMERG;
				}
				if (verbosity >= DEBUG) {
					verbosity = DEBUG;
				}
				break;

			/* dry run */
			case 'x':
				dry_run = 1;
				break;

			case '?':
			case 'h':
			default:
				usage();
		}
	}

	argc -= optind;
	argv += optind;

	api_key = getenv("GANDI_DNS_API_KEY");
	if (api_key == NULL) {
		logmsg(EMERG, "FATAL: ", "Unable to find a value for the Gandi LiveDNS "
			"API Key in the 'GANDI_DNS_API_KEY' environment variable.",
			__FILE__, __LINE__);
		exit(1);
	}

	if (domain == NULL) {
		domain = getenv("GANDI_DNS_DOMAIN");
		if (domain == NULL || strlen(domain) < 3) {
			logmsg(EMERG, "FATAL: ", "Unable to find a value for 'domain' in "
				"either the -d argument or the 'GANDI_DNS_DOMAIN' environment "
				"variable." , __FILE__, __LINE__);
			exit(1);
		}
	}

	logmsg(INFO, "domain=", domain, __FILE__, __LINE__);

	if (subdomain == NULL) {
		subdomain = getenv("GANDI_DNS_SUBDOMAIN");
		if (subdomain == NULL || strlen(subdomain) < 1) {
			logmsg(EMERG, "FATAL: ", "Unable to find a value for 'subdomain' "
				"in either the -s argument or the 'GANDI_DNS_SUBDOMAIN' "
				"environment variable.", __FILE__, __LINE__);
			exit(1);
		}
	}

	logmsg(INFO, "subdomain=", subdomain, __FILE__, __LINE__);

	if (ipv4_lookup_url == NULL) {
		ipv4_lookup_url = IPV4_LOOKUP_URL_DEFAULT;
	}

	logmsg(INFO, "ipv4_lookup_url=", ipv4_lookup_url, __FILE__, __LINE__);

	if (ipv4_lookup_property == NULL) {
		ipv4_lookup_property = IPV4_LOOKUP_PROPERTY_DEFAULT;
	}

	logmsg(INFO, "ipv4_lookup_property=", ipv4_lookup_property,
		__FILE__, __LINE__);

	snprintf(ttl_buffer, TTL_CHAR_BUFSIZE, "%d", ttl);

	if (ttl > LIVEDNS_MAX_TTL) {
		ttl = LIVEDNS_MAX_TTL;
		snprintf(ttl_buffer, TTL_CHAR_BUFSIZE, "%d", ttl);
		logmsg(ERR, "Desired ttl exceeded LIVEDNS_MAX_TTL, capped to ",
			ttl_buffer, __FILE__, __LINE__);
	}

	if (ttl < LIVEDNS_MIN_TTL) {
		ttl = LIVEDNS_MIN_TTL;
		snprintf(ttl_buffer, TTL_CHAR_BUFSIZE, "%d", ttl);
		logmsg(ERR, "Desired ttl lower than LIVEDNS_MIN_TTL, increased to ",
			ttl_buffer, __FILE__, __LINE__);
	}

	snprintf(ttl_buffer, TTL_CHAR_BUFSIZE, "%d", ttl);
	logmsg(INFO, "ttl=", ttl_buffer, __FILE__, __LINE__);

	root = req_get(ipv4_lookup_url, NULL, &last_status);

	fail_hard_if_null(root, "failed to fetch IPv4 address, no parsable JSON "
		"response returned", __FILE__, __LINE__);

	snprintf(last_status_buffer, 4, "%ld", last_status);

	logmsg(DEBUG, "HTTP status from ipv4_lookup_url=", last_status_buffer,
		__FILE__, __LINE__);

	logmsg(DEBUG, "response from ipv4_lookup_url=",
		cJSON_PrintUnformatted(root), __FILE__, __LINE__);

	if (last_status >= 400) {
		logmsg(EMERG, "received error response from ipv4_lookup_url=",
			last_status_buffer, __FILE__, __LINE__);
		exit(1);
	}

	if (cJSON_HasObjectItem(root, ipv4_lookup_property)) {
		ip = cJSON_GetObjectItem(root, ipv4_lookup_property);
		snprintf(current_ipv4, sizeof current_ipv4, "%s",
			cJSON_GetStringValue(ip));
		cJSON_free(ip);
		logmsg(NOTICE, "current_ipv4=", current_ipv4, __FILE__, __LINE__);
	}

	cJSON_free(root);

	/* XXX Use malloc here */
	snprintf(url, sizeof url,
		"https://dns.api.gandi.net/api/v5/domains/%s/records",
		domain);

	logmsg(DEBUG, "url=", url, __FILE__, __LINE__);

	/* XXX Use malloc here */
	snprintf(api_key_header, sizeof api_key_header,
		"X-Api-Key: %s", api_key);

	options = malloc(sizeof(req_options));
	headers[0] = api_key_header;
	headers[1] = NULL;
	options->headers = headers;

	root = req_get(url, options, &last_status);

	fail_hard_if_null(root, "failed to fetch DNS records, no parsable JSON "
		"response returned from LiveDNS", __FILE__, __LINE__);

	snprintf(last_status_buffer, 4, "%ld", last_status);

	logmsg(DEBUG, "HTTP status from LiveDNS GET=",
		last_status_buffer, __FILE__, __LINE__);

	logmsg(DEBUG, "response from LiveDNS GET=",
		cJSON_PrintUnformatted(root), __FILE__, __LINE__);

	if (last_status >= 400) {
		logmsg(EMERG, "received an error response from LiveDNS GET=",
			last_status_buffer, __FILE__, __LINE__);

		item = cJSON_GetObjectItem(root, "message");

		fail_hard_if_null(item, "No error message provided",
			__FILE__, __LINE__);

		logmsg(EMERG, "error=", cJSON_GetStringValue(item),
			__FILE__, __LINE__);

		exit(1);
	}

	cJSON_ArrayForEach(item, root) {
		type = cJSON_GetObjectItem(item, "rrset_type");
		name = cJSON_GetObjectItem(item, "rrset_name");
		values = cJSON_GetObjectItem(item, "rrset_values");

		if (strcmp(cJSON_GetStringValue(type), "A") == 0 &&
			strcmp(cJSON_GetStringValue(name), subdomain) == 0) {

			logmsg(DEBUG, "found matching record: ",
				cJSON_PrintUnformatted(item), __FILE__, __LINE__);

			update_mode = UPDATE;

			cJSON_ArrayForEach(ip, values) {
				if (strcmp(ip->valuestring, current_ipv4) == 0) {
					update_mode = ACCURATE;
				} else {
					logmsg(INFO, "record doesn't have accurate ipv4"
						" address in A record. Stale value=",
						ip->valuestring, __FILE__, __LINE__);
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
			logmsg(INFO, "Record is in the desired state, nothing to do",
				NULL, __FILE__, __LINE__);
			printf("The 'A' record for '%s' is already set to the current "
				"public IPv4 address of '%s'.\nNothing to do.\n",
				subdomain, current_ipv4);
		break;

		case UPDATE:
			logmsg(INFO, "'A' record needs to be updated=",
				subdomain, __FILE__, __LINE__);
			if (dry_run) {
				logmsg(INFO, "Not proceeding with operation as dry_run was set "
					"with -x", NULL, __FILE__, __LINE__);
				printf("The 'A' record for '%s' will be updated with an IPv4 "
					"address of '%s'.\nNot proceeding with operation as the "
					"dry_run option was set with -x.\n", subdomain,
					current_ipv4);
				exit(0);
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

			fail_hard_if_null(root, "failed to update DNS record, no parsable "
				" JSON response returned from LiveDNS", __FILE__, __LINE__);

			snprintf(last_status_buffer, 4, "%ld", last_status);

			logmsg(DEBUG, "HTTP status from LiveDNS PUT=", last_status_buffer,
				__FILE__, __LINE__);

			logmsg(DEBUG, "response from LiveDNS PUT=",
				cJSON_PrintUnformatted(root), __FILE__, __LINE__);

			if (last_status >= 200 && last_status <= 299) {
				logmsg(NOTICE, "new 'A' record update for ",
					subdomain, __FILE__, __LINE__);
				printf("The 'A' record for '%s' was updated to the public IPv4 "
					"address of '%s'.\n", subdomain, current_ipv4);
			} else {
				logmsg(CRIT, "'A' record not update for ",
					subdomain, __FILE__, __LINE__);
				printf("The 'A' record for '%s' was NOT updated to the public "
					"IPv4 address of '%s'. Set increased verbosity to see "
					"details and try again.\n", subdomain, current_ipv4);
			}

			cJSON_free(new_array);
			cJSON_free(new_obj);
		break;

		case CREATE:
			logmsg(INFO, "'A' record needs to be created=", subdomain,
				__FILE__, __LINE__);

			if (dry_run) {
				logmsg(INFO, "Not proceeding with operation as dry_run was set "
					"with -x", NULL, __FILE__, __LINE__);
				printf("A new 'A' record will be created for '%s' with an IPv4 "
					"address of '%s'.\nNot proceeding with operation as the "
					"dry_run option was set with -x.\n",
					subdomain, current_ipv4);
				exit(0);
			}

			new_ip_array[0] = current_ipv4;
			new_obj = cJSON_CreateObject();
			new_array = cJSON_CreateStringArray(new_ip_array, 1);

			cJSON_AddItemToObject(new_obj, "rrset_values", new_array);
			cJSON_AddStringToObject(new_obj, "rrset_name", subdomain);
			cJSON_AddStringToObject(new_obj, "rrset_type", "A");
			cJSON_AddNumberToObject(new_obj, "rrset_ttl", ttl);

			logmsg(DEBUG, "JSON to be used for record creation=",
					cJSON_PrintUnformatted(new_obj), __FILE__, __LINE__);

			root = req_post(url, new_obj, options, &last_status);

			fail_hard_if_null(root, "failed to create DNS record, no parsable "
				"JSON response returned from LiveDNS", __FILE__, __LINE__);

			snprintf(last_status_buffer, 4, "%ld", last_status);

			logmsg(DEBUG, "HTTP status from LiveDNS POST=",
				last_status_buffer, __FILE__, __LINE__);

			logmsg(DEBUG, "response from LiveDNS POST=",
				cJSON_PrintUnformatted(root), __FILE__, __LINE__);

			if (last_status >= 200 && last_status <= 299) {
				logmsg(NOTICE, "new 'A' record created for ", subdomain,
					__FILE__, __LINE__);
				printf("An 'A' record for '%s' was created with the public IPv4"
					" address of '%s'.\n", subdomain, current_ipv4);
			} else {
				logmsg(CRIT, "'A' record not created for ",
					subdomain, __FILE__, __LINE__);
				printf("An 'A' record for '%s' was NOT created with the public "
					"IPv4 address of '%s'. Set increased verbosity to see more "
					"details and try again.\n\n", subdomain, current_ipv4);
			}

			cJSON_free(new_array);
			cJSON_free(new_obj);
		break;
	}
	return 0;
}

static void
logmsg(int level, const char * msg, const char * value, const char * file,
	unsigned int line)
{
	char * severity;
	if (level <= verbosity) {
		switch (level) {
			case EMERG:
				severity = "EMERG";
				break;
			case ALERT:
				severity = "ALERT";
				break;
			case CRIT:
				severity = "CRIT";
				break;
			case ERR:
				severity = "ERR";
				break;
			case WARN:
				severity = "WARN";
				break;
			case NOTICE:
				severity = "NOTICE";
				break;
			case INFO:
				severity = "INFO";
				break;
			case DEBUG:
				severity = "DEBUG";
				break;
		}
		if (value == NULL) {
			value = "";
		}
		fprintf(stderr, "%s,%s:%d,%s%s\n", severity, file, line, msg, value);
	}
}

static void
usage()
{
	fprintf(stderr, "Usage:\n  dldns [-xh] [-i ipv4 lookup] [-p json prop] "
	"[-t ttl] [-v verbosity] -s subdomain -d domain\n");
	exit(1);
}

static void
fail_hard_if_null(void * ptr, const char * msg, const char * file,
	unsigned int line)
{
	if (ptr == NULL) {
		if (msg == NULL) {
			msg = "malloc failed";
		}
		logmsg(EMERG, msg, NULL, file, line);
		exit(1);
	}
}
