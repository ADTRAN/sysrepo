/**
 * @file test_modules2.c
 * @author Michal Vasko <mvasko@cesnet.cz>
 * @brief test for adding/removing modules part2
 *
 * @copyright
 * Copyright 2018 Deutsche Telekom AG.
 * Copyright 2018 - 2019 CESNET, z.s.p.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define _GNU_SOURCE

#include <unistd.h>
#include <setjmp.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>

#include <cmocka.h>
#include <libyang/libyang.h>

#include "tests/config.h"
#include "sysrepo.h"

struct state {
    sr_conn_ctx_t *conn;
};

static int
setup_f(void **state)
{
    struct state *st;

    st = calloc(1, sizeof *st);
    *state = st;

    if (sr_connect(0, &st->conn) != SR_ERR_OK) {
        return 1;
    }

    return 0;
}

static int
teardown_f(void **state)
{
    struct state *st = (struct state *)*state;

    sr_disconnect(st->conn);
    free(st);
    return 0;
}

static void
test_feature_dependencies_across_modules(void **state)
{
    struct state *st = (struct state *)*state;
    int ret;
    uint32_t conn_count;

    /* install modules */
    ret = sr_install_module(st->conn, TESTS_DIR "/files/feature-deps.yang", TESTS_DIR "/files", NULL, 0);
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_install_module(st->conn, TESTS_DIR "/files/feature-deps2.yang", TESTS_DIR "/files", NULL, 0);
    assert_int_equal(ret, SR_ERR_OK);

    /* apply scheduled changes */
    sr_disconnect(st->conn);
    st->conn = NULL;
    ret = sr_connection_count(&conn_count);
    assert_int_equal(ret, SR_ERR_OK);
    assert_int_equal(conn_count, 0);
    ret = sr_connect(SR_CONN_ERR_ON_SCHED_FAIL, &st->conn);
    assert_int_equal(ret, SR_ERR_OK);

    /* enable independent feature */
    ret = sr_enable_module_feature(st->conn, "feature-deps2", "featx");
    assert_int_equal(ret, SR_ERR_OK);

    /* apply scheduled changes */
    sr_disconnect(st->conn);
    st->conn = NULL;
    ret = sr_connection_count(&conn_count);
    assert_int_equal(ret, SR_ERR_OK);
    assert_int_equal(conn_count, 0);
    ret = sr_connect(SR_CONN_ERR_ON_SCHED_FAIL, &st->conn);
    assert_int_equal(ret, SR_ERR_OK);

    /* enable dependent features */
    ret = sr_enable_module_feature(st->conn, "feature-deps", "feat1");
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_enable_module_feature(st->conn, "feature-deps", "feat2");
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_enable_module_feature(st->conn, "feature-deps", "feat3");
    assert_int_equal(ret, SR_ERR_OK);

    /* apply scheduled changes */
    sr_disconnect(st->conn);
    st->conn = NULL;
    ret = sr_connection_count(&conn_count);
    assert_int_equal(ret, SR_ERR_OK);
    assert_int_equal(conn_count, 0);
    ret = sr_connect(SR_CONN_ERR_ON_SCHED_FAIL, &st->conn);
    assert_int_equal(ret, SR_ERR_OK);

    /* check if modules can be loaded again */
    sr_disconnect(st->conn);
    st->conn = NULL;
    ret = sr_connection_count(&conn_count);
    assert_int_equal(ret, SR_ERR_OK);
    assert_int_equal(conn_count, 0);
    ret = sr_connect(0, &st->conn);
    assert_int_equal(ret, SR_ERR_OK);

    ret = sr_remove_module(st->conn, "feature-deps");
    assert_int_equal(ret, SR_ERR_OK);
    ret = sr_remove_module(st->conn, "feature-deps2");
    assert_int_equal(ret, SR_ERR_OK);
}

int
main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_feature_dependencies_across_modules, setup_f, teardown_f),
    };

    setenv("CMOCKA_TEST_ABORT", "1", 1);
    sr_log_stderr(SR_LL_INF);
    return cmocka_run_group_tests(tests, NULL, NULL);
}
