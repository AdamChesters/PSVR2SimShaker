#include "feedback_contract.hpp"
#include <iostream>
using namespace shaker;
void expect(bool value){if(!value)throw std::runtime_error("Feedback contract check failed");}
int main(){
    const auto body=feedbackPayload(" Adam ","adam@example.test "," An idea ","0.3.0-alpha.5");
    expect(body.size()==5&&body["app"]=="PSVR2SimShaker"&&body["version"]=="0.3.0-alpha.5"&&body["message"]=="An idea");
    for(const auto& fields:std::vector<std::array<std::string,3>>{{"","a@b.test","idea"},{std::string(101,'a'),"a@b.test","idea"},{"Adam","bad","idea"},{"Adam","a@@b.test","idea"},{"Adam","a@b.test"," "},{"Adam","a@b.test",std::string(4001,'a')}}){
        bool rejected=false;try{feedbackPayload(fields[0],fields[1],fields[2],"test");}catch(...){rejected=true;}expect(rejected);
    }
    expect(feedbackAccepted(200,"{\"ok\":true}"));
    for(const auto& response:{"{}","{\"ok\":false}","{\"ok\":\"true\"}","bad"})expect(!feedbackAccepted(200,response));
    expect(!feedbackAccepted(500,"{\"ok\":true}"));
    std::cout<<"Feedback metadata, validation and explicit relay acceptance verified.\n";
}
