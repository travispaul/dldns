#include <atf-c.h>

#include "../req.h"

ATF_TC(GET);
ATF_TC_HEAD(GET, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test GET request");
}
ATF_TC_BODY(GET, tc)
{
	cJSON * root;
	long status;

	root = req_get("https://ifconfig.co/json", NULL, &status);

	ATF_CHECK_EQ(status, 200);
	ATF_CHECK(root != NULL);

	cJSON_free(root);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, GET);
	return atf_no_error();
}
