#pragma once

#define TEEP_AGENT_BROKER_TASK_DONE 1
#define TEEP_AGENT_QUERY_NEXT_BROKER_TASK 2
#define TEEP_AGENT_SET_DEV_OPTION 3

enum broker_command
{
    BROKER_HTTP_POST,
    BROKER_HTTP_GET,
    BROKER_FINISH
};

#define TEEP_MAX_MESSAGE_LEN 2048
#define TEEP_MAX_URI_LEN 512

struct broker_task
{
    enum broker_command command;
    char uri[TEEP_MAX_URI_LEN];
    char post_data[TEEP_MAX_MESSAGE_LEN];
    size_t post_data_len;
};

enum agent_dev_option
{
    AGENT_OPTION_SET_TAM_URI,
    AGENT_OPTION_SET_CURRENT_TA_LIST
};
