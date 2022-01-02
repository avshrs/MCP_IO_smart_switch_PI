#include "MCP_Mosquitto.h"
#include "MCP_Manager.h"

#include <iostream>
#include <iomanip>
#include <ctime>
#include <vector>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

mqtt_client::mqtt_client(const char *id, const char *host, int port) : mosquittopp(id)
{
    int keepalive = 60;
    connect(host, port, keepalive);
    
}

void mqtt_client::client_loop_forever(){
    if(mcp_manager != NULL){
        reconnect_delay_set(5, 1000000, true);
        loop_forever();
    }
    else{
        std::cout << "Wrong class constructor!!!" << std::endl;
    }
}


void mqtt_client::register_subs(){
    subscribe(NULL, "MCP_Array");  // Main device topic - Online 

    for(int i = 0; i<64; i++){
        std::string pub = "MCP_OUT_P_";
        std::string sub = "MCP_OUT_S_";
        sub += std::to_string(i);
        pub += std::to_string(i);
        subscribe(NULL, sub.c_str());
        usleep(10000);
    }
}

void mqtt_client::unregister_subs(){
    unsubscribe(NULL, "MCP_Array");  // Main device topic - Online 
    for(int i = 0; i<64; i++){
        std::string pub = "MCP_OUT_P_";
        std::string sub = "MCP_OUT_S_";
        sub += std::to_string(i);
        pub += std::to_string(i);
        
        unsubscribe(NULL, sub.c_str());
        usleep(10000);
    }
}

void mqtt_client::on_error() {
    std::cout<<"onerror"<<std::endl;
    return;}


void mqtt_client::on_connect(int rc)
{
    if (!rc)
    {

        #ifdef DEBUG
            auto t = std::time(nullptr);
            auto tm = *std::localtime(&t);      
            std::cout << std::put_time(&tm, "%d-%m-%Y %H-%M-%S | ");
            std::cout << "Connected with code " << rc << std::endl;
        #endif
        unregister_subs();
        register_subs();
    }
    
}

void mqtt_client::on_disconnect(int rc)
{
    if (!rc)
    {
        #ifdef DEBUG
            auto t = std::time(nullptr);
            auto tm = *std::localtime(&t);      
            std::cout << std::put_time(&tm, "%d-%m-%Y %H-%M-%S | ");
            std::cout << "disconnected - code " << rc << std::endl;
        #endif
    }
}
void mqtt_client::on_subscribe(int mid, int qos_count, const int *granted_qos)
{

    #ifdef DEBUG
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);      
        std::cout << std::put_time(&tm, "%d-%m-%Y %H-%M-%S | ");
        std::cout << "Subscription succeeded. " << " mid: " << mid << " qos_count: "<< qos_count << " qos_granted: "<< granted_qos << std::endl;
    #endif
}


void mqtt_client::register_mcp_manager(MCP_Manager *mcp_manager_){
    mcp_manager = mcp_manager_;
}

void mqtt_client::send_ack(std::string pub, std::string msg ){
    publish(NULL, pub.c_str(), msg.length(), msg.c_str());
}

void mqtt_client::pub_state(int out, bool state){
    std::string pub = "MCP_OUT_P_";
    pub += std::to_string(out);
    std::string msg; 
    if (state){
        msg = "ON";
    }
    else{
        msg = "OFF";
    }
    publish(NULL, pub.c_str(), msg.length(), msg.c_str());
}

void mqtt_client::on_message(const struct mosquitto_message *message){

    try{
        std::string message_topic(message->topic);
        std::string message_payload(static_cast<char*>(message->payload));
        if(!message_payload.empty() && message_topic.find("MCP_OUT_S_") != std::string::npos){
            std::vector<std::string> tdata = parse_string(message_topic, '_');
            if (tdata.size() != 4){
                auto t = std::time(nullptr);
                auto tm = *std::localtime(&t);      
                std::cout << std::put_time(&tm, "%d-%m-%Y %H-%M-%S | ");
                std::cout << "Wrong Topic lengh" << std::endl;
            }
            else if(message_payload == "ON"){
                int nr = std::stoi(tdata[3]);
                mcp_manager->write_output(nr, true, 999);
                #ifdef DEBUG
                    auto t = std::time(nullptr);
                    auto tm = *std::localtime(&t);      
                    std::cout << std::put_time(&tm, "%d-%m-%Y %H-%M-%S | ");
                    std::cout << "Request to turn on output nr:" << nr<<std::endl;
                #endif
            }
            else if(message_payload.find("ONTIME")!=std::string::npos){
                std::vector<std::string> mdata = parse_string(message_payload, '_');
                if (mdata.size() != 3 ){
                    auto t = std::time(nullptr);
                    auto tm = *std::localtime(&t);      
                    std::cout << std::put_time(&tm, "%d-%m-%Y %H-%M-%S | ");
                    std::cout << "Wrong payload lengh" << std::endl;
                }
                else{
                    int nr = std::stoi(tdata[3]);
                    int time = std::stoi(mdata[1]);
                    int force = std::stoi(mdata[2]);
                    mcp_manager->write_output_timer(nr, time, force);
                    #ifdef DEBUG
                        auto t = std::time(nullptr);
                        auto tm = *std::localtime(&t);      
                        std::cout << std::put_time(&tm, "%d-%m-%Y %H-%M-%S | ");
                        std::cout << "Request to turn on output nr:" << nr << " by PIR with timeout: " << time << " seconds" << std::endl;
                    #endif
                }
                
            }

            else if(message_payload == "OFF"){
                int nr = std::stoi(tdata[3]);
                mcp_manager->write_output(nr, false, 999);
                #ifdef DEBUG
                    auto t = std::time(nullptr);
                    auto tm = *std::localtime(&t);      
                    std::cout << std::put_time(&tm, "%d-%m-%Y %H-%M-%S | ");
                    std::cout << "Request to turn off output nr:" << nr<<std::endl;
                #endif
            }
        }
    }
    catch(...){std::cout << "Received Empty Payload" << std:: endl;}
}


std::vector<std::string> mqtt_client::parse_string(std::string str, char delimiter){
    std::vector<std::string> vect;
    std::stringstream ss(str);

    while (ss.good()) {
        std::string substr;
        getline(ss, substr, delimiter);
        vect.push_back(substr);
    }
    // std::cout << vect.size() << std::endl;
    return vect;
}
