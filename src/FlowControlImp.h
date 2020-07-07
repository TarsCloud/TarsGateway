#ifndef _H_FLOWCONTROLIMP_H_
#define _H_FLOWCONTROLIMP_H_

#include "servant/Application.h"
#include "FlowControl.h"

class FlowControlImp: public Base::FlowControl
{

public:
    FlowControlImp(/* args */);
    virtual ~FlowControlImp();
    virtual void initialize();

    /**
     *
     */
    virtual void destroy();
    int report(const map<string, int>& flow, const string& ip, tars::TarsCurrentPtr current);

    int getGWDB(map<string, string>& dbConf, tars::TarsCurrentPtr current);

  private:
    /* data */

};



#endif // !1 _H_FLOWCONTROLIMP_H_
