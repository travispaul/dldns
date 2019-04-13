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

#include <curl/curl.h>

#include "req.h"

static size_t
write_mem_callback(void * contents, size_t size, size_t nmemb,
	void *userp)
{
	size_t realsize = size * nmemb;
	req_mem *mem = (req_mem *)userp;

	char *ptr = realloc(mem->memory, mem->size + realsize + 1);
	if(ptr == NULL) {
		/* out of memory! */ 
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

static size_t
read_mem_callback(void * dest, size_t size, size_t nmemb, void *userp)
{
	req_mem *wt = (req_mem *)userp;
	size_t buffer_size = size*nmemb;

	if(wt->size) {
		/* copy as much as possible from the source to the destination */ 
		size_t copy_this_much = wt->size;
		if(copy_this_much > buffer_size)
		copy_this_much = buffer_size;
		memcpy(dest, wt->memory, copy_this_much);

		wt->memory += copy_this_much;
		wt->size -= copy_this_much;
		return copy_this_much; /* we copied this many bytes */ 
	}

	return 0; /* no more data left to deliver */ 
}

cJSON *
req_put_or_post(CURLoption method, const char * url, cJSON * body,
	req_options * options, long * status)
{
	CURL *curl_handle;
	CURLcode res;
	struct curl_slist *list = NULL;
	req_mem chunk;
	req_mem read_chunk;
	cJSON *root;
	root = NULL;
	char * data;

	data = cJSON_PrintUnformatted(body);

	chunk.memory = malloc(1);
	chunk.size = 0;

	read_chunk.memory = data;
	read_chunk.size = strlen(data);

	curl_global_init(CURL_GLOBAL_ALL);

	curl_handle = curl_easy_init();

	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	curl_easy_setopt(curl_handle, method, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_mem_callback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
	curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, read_mem_callback);
	curl_easy_setopt(curl_handle, CURLOPT_READDATA, (void *)&read_chunk);
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, read_chunk.size);
	curl_easy_setopt(curl_handle, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

	list = curl_slist_append(list, "Content-Type: application/json");
	list = curl_slist_append(list, "Expect:");

	if (options != NULL) {
		if (options->headers != NULL) {
			int i = 0;
			while (options->headers[i] != NULL) {
				list = curl_slist_append(list, options->headers[i]);
				i += 1;
			}
			curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, list);
		}
	}

	res = curl_easy_perform(curl_handle);

	if(res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
		curl_easy_strerror(res));
	}
	else {
		root = cJSON_Parse(chunk.memory);
	}

	curl_easy_cleanup(curl_handle);

	curl_slist_free_all(list);

	free(chunk.memory);

	curl_global_cleanup();

	return root;
}

cJSON *
req_get(const char * url, req_options * options, long * status)
{
	CURL *curl_handle;
	CURLcode res;
	struct curl_slist *list = NULL;
	req_mem chunk;
	cJSON *root;
	root = NULL;

	chunk.memory = malloc(1);
	chunk.size = 0;

	curl_global_init(CURL_GLOBAL_ALL);

	curl_handle = curl_easy_init();

	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_mem_callback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_setopt(curl_handle, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

	if (options != NULL) {
		if (options->headers != NULL) {
			int i = 0;
			while (options->headers[i] != NULL) {
				list = curl_slist_append(list, options->headers[i]);
				i += 1;
			}
			curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, list);
		}
	}

	res = curl_easy_perform(curl_handle);

	if (res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
		curl_easy_strerror(res));
	} else {
		curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, status);
		root = cJSON_Parse(chunk.memory);
	}

	curl_easy_cleanup(curl_handle);

	if (list != NULL) {
		curl_slist_free_all(list);
	}

	free(chunk.memory);

	curl_global_cleanup();

	return root;
}

cJSON *
req_put(const char * url, cJSON * body, req_options * options,
	long * status)
{
	return req_put_or_post(CURLOPT_PUT, url, body, options, status);
}

cJSON *
req_post(const char *url, cJSON * body, req_options * options,
	long * status)
{
	return req_put_or_post(CURLOPT_POST, url, body, options, status);
}