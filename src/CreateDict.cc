#include "DictProducer.h"
#include "Config.h"
#include "SplitToolCppJieba.h"

int main(){
    SplitToolCppJieba* tool = SplitToolCppJieba::getInstance();
    DictProducer producer(tool);
    producer.buildDict();
    producer.createIndex();
    producer.store();
    return 0;
}