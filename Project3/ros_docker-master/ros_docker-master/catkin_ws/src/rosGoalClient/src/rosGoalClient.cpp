#include <actionlib/client/simple_action_client.h>
#include <arpa/inet.h>
#include <move_base_msgs/MoveBaseAction.h>
#include <mysql/mysql.h>
#include <pthread.h>
#include <ros/ros.h>
#include <signal.h>
#include <std_srvs/SetBool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "rosGoalClient/bot3gpio.h"

#define NODE_NAME "rosGoalClient"
#define SRV_NAME "bot3_gpio"

#define BUF_SIZE 100
#define NAME_SIZE 20
#define ARR_CNT 10
#define GOALCNT 3

typedef actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction>
    MoveBaseClient;

void *send_msg(void *arg);
void *recv_msg(void *arg);
void error_handling(const char *msg);
int SprayIc(int k);

char name[NAME_SIZE] = "[Default]";
char msg[BUF_SIZE];
pthread_mutex_t g_mutex;

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread, mysql_thread;
    void *thread_return;

    ros::init(argc, argv, NODE_NAME);
    if (argc != 4) {
        printf("Usage :rosrun rosGoalClient %s <IP> <port> <name>\n", argv[0]);
        exit(1);
    }

    pthread_mutex_init(&g_mutex, NULL);

    sprintf(name, "%s", argv[3]);

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");
    sprintf(msg, "[%s:PASSWD]", name);
    write(sock, msg, strlen(msg));

    pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock);
    pthread_create(&snd_thread, NULL, send_msg, (void *)&sock);

    pthread_join(snd_thread, &thread_return);
    pthread_join(rcv_thread, &thread_return);

    close(sock);
    return 0;
}

void *send_msg(void *arg) {
    int *sock = (int *)arg;
    int str_len;
    int ret;
    fd_set initset, newset;
    struct timeval tv;
    char name_msg[NAME_SIZE + BUF_SIZE + 2];

    FD_ZERO(&initset);
    FD_SET(STDIN_FILENO, &initset);

    fputs("Input a message! [ID]msg (Default ID:ALLMSG)\n", stdout);
    while (1) {
        memset(msg, 0, sizeof(msg));
        name_msg[0] = '\0';
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        newset = initset;
        ret = select(STDIN_FILENO + 1, &newset, NULL, NULL, &tv);
        if (FD_ISSET(STDIN_FILENO, &newset)) {
            fgets(msg, BUF_SIZE, stdin);
            if (!strncmp(msg, "quit\n", 5)) {
                *sock = -1;
                return NULL;
            } else if (msg[0] != '[') {
                strcat(name_msg, "[ALLMSG]");
                strcat(name_msg, msg);
            } else
                strcpy(name_msg, msg);
            if (write(*sock, name_msg, strlen(name_msg)) <= 0) {
                *sock = -1;
                return NULL;
            }
        }
        if (ret == 0) {
            if (*sock == -1) return NULL;
        }
    }
}

void *recv_msg(void *arg) {
    MYSQL *conn;
    //	MYSQL_RES* res_ptr;
    MYSQL_ROW sqlrow;
    char sql_cmd[200] = {0};
    int res;

    const char *host = "10.10.141.133";
    const char *user = "iot";
    const char *pass = "pwiot";
    const char *dbname = "iotdb";

    double dSeqVal[GOALCNT][3] = {
        {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0},
    };

    int *sock = (int *)arg;
    int i, spray, count = 0;
    char *pToken;
    char *pArray[ARR_CNT] = {0};

    char name_msg[NAME_SIZE + BUF_SIZE + 1];
    int str_len;

    conn = mysql_init(NULL);

    puts("MYSQL startup");
    if (!(mysql_real_connect(conn, host, user, pass, dbname, 0, NULL, 0))) {
        fprintf(stderr, "ERROR : %s[%d]\n", mysql_error(conn),
                mysql_errno(conn));
        exit(1);
    } else
        printf("Connection Successful!\n\n");

    ros::NodeHandle nh;

    ros::ServiceClient srv_client;
    srv_client = nh.serviceClient<rosGoalClient::bot3gpio>(SRV_NAME);
    rosGoalClient::bot3gpio srv;
    srv.request.a = 0;
    srv.request.b = 1;

    MoveBaseClient ac("move_base", true);
    move_base_msgs::MoveBaseGoal goal;

    while (!ac.waitForServer(ros::Duration(5.0))) {
        ROS_INFO("Waiting for the move_base action server to come up");
    }

    goal.target_pose.header.frame_id = "map";

    while (1) {
        pthread_mutex_lock(&g_mutex);
        memset(name_msg, 0x0, sizeof(name_msg));
        str_len = read(*sock, name_msg, NAME_SIZE + BUF_SIZE);
        if (str_len <= 0) {
            *sock = -1;
            return NULL;
        }
        name_msg[str_len - 1] = '\0';

        fputs(name_msg, stdout);
        printf("\n");

        pToken = strtok(name_msg, "[:@]");
        i = 0;

        while (pToken != NULL) {
            pArray[i] = pToken;
            if (i++ >= ARR_CNT) break;
            pToken = strtok(NULL, "[:@]");
        }

        if (!strcmp(pArray[1], "SPRAYIC")) {
            srv.request.a = 3;

            ROS_INFO("send srv, srv.request.a : %ld", srv.request.a);
            if (srv_client.call(srv)) {
                ROS_INFO("recieve srv, srv.Response.result: %ld",
                         srv.response.result);

                sprintf(name_msg, "[%s]SPRAYSCCUESS\n", pArray[0]);
                write(*sock, name_msg, strlen(name_msg));

                srv.request.a = 4;
            } else {
                ROS_ERROR("Failed to Spray");
                srv.request.a = 4;

                sprintf(name_msg, "[%s]SPRAYFAIL\n", pArray[0]);
                write(*sock, name_msg, strlen(name_msg));
            }
        } else if (!strcmp(pArray[1], "GOGOAL") && (i == 5)) {
            goal.target_pose.header.stamp = ros::Time::now();

            goal.target_pose.pose.position.x = strtod(pArray[2], NULL);
            goal.target_pose.pose.position.y = strtod(pArray[3], NULL);
            goal.target_pose.pose.orientation.w = strtod(pArray[4], NULL);
            ROS_INFO("Go target point");
            ac.sendGoalAndWait(goal, ros::Duration(120.0, 0),
                               ros::Duration(120.0, 0));

            if (ac.getState() == actionlib::SimpleClientGoalState::SUCCEEDED) {
                ROS_INFO("Point arrived!");

                srv.request.a = 3;

                ROS_INFO("send srv, srv.request.a : %ld", srv.request.a);
                if (srv_client.call(srv)) {
                    ROS_INFO("recieve srv, srv.Response.result: %ld",
                             srv.response.result);
                    ROS_INFO("SPRAY ON");
                    srv.request.a = 4;
                    sprintf(sql_cmd,
                            "insert into BugBot(spraying,xlocation,ylocation, "
                            "working, date,time) values(\"spray "
                            "complete\",\"%s\",\"%s\",\"Good "
                            "operation\",now(),now())",
                            pArray[2], pArray[3]);

                    res = mysql_query(conn, sql_cmd);
                    if (!res)
                        printf("inserted %lu rows\n",
                               (unsigned long)mysql_affected_rows(conn));
                    else
                        fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn),
                                mysql_errno(conn));
                } else {
                    ROS_INFO("recieve srv, srv.Response.result: %ld",
                             srv.response.result);
                    ROS_INFO("SPRAY FAIL");

                    sprintf(
                        sql_cmd,
                        "insert into BugBot(spraying, working, date,time) "
                        "values(\"spray fail\",\"spray error\",now(),now())");

                    res = mysql_query(conn, sql_cmd);
                    if (!res)
                        printf("inserted %lu rows\n",
                               (unsigned long)mysql_affected_rows(conn));
                    else
                        fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn),
                                mysql_errno(conn));

                    srv.request.a = -1;
                }

                if (srv.request.a == 4) {
                    srv.request.a = 2;
                    ROS_INFO("send srv, srv.request.a : %ld", srv.request.a);
                    if (srv_client.call(srv)) {
                        ROS_INFO("recieve srv, srv.Response.result: %ld",
                                 srv.response.result);
                        ROS_INFO("SPRAY OFF");

                        srv.request.a = 4;

                    } else {
                        ROS_INFO("recieve srv, srv.Response.result: %ld",
                                 srv.response.result);
                        ROS_INFO("SPRAY FAIL");

                        sprintf(sql_cmd,
                                "insert into BugBot(spraying, working, "
                                "date,time) values(\"spray off fail\",\"spray "
                                "error\",now(),now())");

                        res = mysql_query(conn, sql_cmd);
                        if (!res)
                            printf("inserted %lu rows\n",
                                   (unsigned long)mysql_affected_rows(conn));
                        else
                            fprintf(stderr, "ERROR: %s[%d]\n",
                                    mysql_error(conn), mysql_errno(conn));

                        srv.request.a = -1;
                    }
                }

                goal.target_pose.header.stamp = ros::Time::now();

                goal.target_pose.pose.position.x = 0.0;
                goal.target_pose.pose.position.y = 0.0;
                goal.target_pose.pose.orientation.w = 1.0;
                ROS_INFO("GO HOME!");
                ac.sendGoalAndWait(goal, ros::Duration(120.0, 0),
                                   ros::Duration(120.0, 0));

                if (ac.getState() ==
                    actionlib::SimpleClientGoalState::SUCCEEDED) {
                    ROS_INFO("Arrived Home!");
                    if (srv.request.a == 4) {
                        sprintf(name_msg, "[%s]MISSIONCOMPLETE\n", pArray[0]);
                        write(*sock, name_msg, strlen(name_msg));
                    } else {
                        sprintf(name_msg, "[%s]SPRAYFAIL\n", pArray[0]);
                        write(*sock, name_msg, strlen(name_msg));
                    }
                } else {
                    ROS_INFO("The base failed to move to goal for some season");

                    sprintf(sql_cmd,
                            "insert into BugBot(spraying, working, date,time) "
                            "values(\"spray complete\",\"move "
                            "error\",now(),now())");

                    res = mysql_query(conn, sql_cmd);
                    if (!res)
                        printf("inserted %lu rows\n",
                               (unsigned long)mysql_affected_rows(conn));
                    else
                        fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn),
                                mysql_errno(conn));

                    sprintf(name_msg, "[%s]RETURNFAIL\n", pArray[0]);
                    write(*sock, name_msg, strlen(name_msg));
                }

            } else {
                ROS_INFO("The base failed to move to goal for some reason");
                ROS_INFO("retry!");

                sprintf(sql_cmd,
                        "insert into BugBot(spraying, working, date,time) "
                        "values(\"spray fail\",\"move error\",now(),now())");

                res = mysql_query(conn, sql_cmd);
                if (!res)
                    printf("inserted %lu rows\n",
                           (unsigned long)mysql_affected_rows(conn));
                else
                    fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn),
                            mysql_errno(conn));

                goal.target_pose.header.stamp = ros::Time::now();

                goal.target_pose.pose.position.x = 0.0;
                goal.target_pose.pose.position.y = 0.0;
                goal.target_pose.pose.orientation.w = 1.0;
                ROS_INFO("GO HOME!");
                ac.sendGoalAndWait(goal, ros::Duration(120.0, 0),
                                   ros::Duration(120.0, 0));

                if (ac.getState() ==
                    actionlib::SimpleClientGoalState::SUCCEEDED) {
                    ROS_INFO("Arrived Home!");
                    sprintf(name_msg, "[%s]MISSIONFAIL\n", pArray[0]);
                    write(*sock, name_msg, strlen(name_msg));

                } else {
                    ROS_INFO("The base failed to move to goal for some season");
                    ROS_INFO("WARNNING..");
                    sprintf(name_msg, "[%s]RETURNFAIL\n", pArray[0]);
                    write(*sock, name_msg, strlen(name_msg));
                }
            }

        } else if (!strcmp(pArray[1], "GOHOME")) {
            goal.target_pose.header.stamp = ros::Time::now();

            goal.target_pose.pose.position.x = 0.0;
            goal.target_pose.pose.position.y = 0.0;
            goal.target_pose.pose.orientation.w = 1.0;
            ROS_INFO("Go Home");
            ac.sendGoalAndWait(goal, ros::Duration(120.0, 0),
                               ros::Duration(120.0, 0));

            if (ac.getState() == actionlib::SimpleClientGoalState::SUCCEEDED) {
                ROS_INFO("Arraived Home!");
                sprintf(name_msg, "[ALLMSG]ARRIVEDHOME\n");
                write(*sock, name_msg, strlen(name_msg));

            } else {
                ROS_INFO("error: Fail to home");
                sprintf(name_msg, "[ALLMSG]FAILTOHOME\n");
                write(*sock, name_msg, strlen(name_msg));
            }

        } else if (!strcmp(pArray[1], "GO") && (i == 5)) {
            goal.target_pose.header.stamp = ros::Time::now();

            goal.target_pose.pose.position.x = strtod(pArray[2], NULL);
            goal.target_pose.pose.position.y = strtod(pArray[3], NULL);
            goal.target_pose.pose.orientation.w = strtod(pArray[4], NULL);
            ROS_INFO("Go target point");
            ac.sendGoalAndWait(goal, ros::Duration(120.0, 0),
                               ros::Duration(120.0, 0));

            if (ac.getState() == actionlib::SimpleClientGoalState::SUCCEEDED) {
                ROS_INFO("point arrived!");
                sprintf(name_msg, "[%s]POINTARRIVED\n", pArray[0]);
                write(*sock, name_msg, strlen(name_msg));

            } else {
                ROS_INFO("error!");
                sprintf(name_msg, "[%s]ERROR\n", pArray[0]);
                write(*sock, name_msg, strlen(name_msg));
            }
        }
        pthread_mutex_unlock(&g_mutex);
        usleep(2);
    }
    mysql_close(conn);
    return 0;
}

void error_handling(const char *msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
