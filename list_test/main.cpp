#include <iostream>
#include <list>

using namespace std;

int visibleImg, infraredImg;


void test(int visibleBufCount, int infraredBufCount, list<int> visiblelist, list<int> infrarelist){
    int visibletemp;
    int infraretemp;
    
    if(visibleBufCount > 0){
        int visitListSize = visiblelist.size();
        cout << "visitListSize : " << visitListSize << endl;

        // 查缓存列表的大小, 如果超过设定的缓存数量,则移除列表中的第一个图像,并将当前可见光图像存储为列表中的最后一个图像
        if(visitListSize >= visibleBufCount){
            visibletemp = visiblelist.front();
            cout << "visibletemp : " << visibletemp << endl;

            visibleImg = visibletemp;
            cout << "visibleImg : " << visibleImg << endl;

            visiblelist.pop_front();
            for (const auto& visible : visiblelist) {
                std::cout << visible << " ";
            }
            std::cout << std::endl;
        }
        visiblelist.push_back(visibleImg);
        for (const auto& visible : visiblelist) {
            std::cout << visible << " ";
        }
        std::cout << std::endl;
    }

    if(infraredBufCount > 0){
        int infraredListSize = infrarelist.size();

        // // 查缓存列表的大小, 如果超过设定的缓存数量,则移除列表中的第一个图像,并将当前红外图像存储为列表中的最后一个图像
        if(infraredListSize >= infraredBufCount){
            infraretemp = infrarelist.front();
            infraredImg = infraretemp;
            infrarelist.pop_front();
        }
        infrarelist.pop_front();
    }
}


int main(){
    int visibleBufCount = 0;
    int infraredBufCount = 0;

    list<int> visiblelist;
    list<int> infrarelist;

    visiblelist.push_back(1);
    visiblelist.push_back(2);
    visiblelist.push_back(3);
    visiblelist.push_front(4);

    infrarelist.push_back(1);
    infrarelist.push_back(2);
    infrarelist.push_back(3);

    test(2, 0, visiblelist, infrarelist);

    return 0;
}