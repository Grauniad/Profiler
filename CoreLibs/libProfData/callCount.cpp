#include "callCount.h"
#include "nodeConfig.h"
#include <algorithm>
#include <utility>
#include <iomanip>
using namespace std;

CallCount::Calls::Calls() 
    : calls(0), costs(NodeConfig::Instance().NewCost())
{
}

CallCount::Calls::Calls(const Calls& rhs) 
    : costs(NodeConfig::Instance().NewCost())
{
    (*this) = rhs;
}

CallCount::Calls::Calls(Calls&& moveFrom) 
    : calls(moveFrom.calls), costs(moveFrom.costs)
{
    moveFrom.costs = nullptr;
}

CallCount::Calls& CallCount::Calls::operator=(const Calls& rhs)
{
    if ( costs == nullptr ) {
        costs = NodeConfig::Instance().NewCost();
    }
    calls = rhs.calls;
    for( size_t i = 0; i< costs->Size(); ++i ) {
        (*costs)[i] = (*rhs.costs)[i];
    }
    return *this;
}

CallCount::Calls::~Calls() {
    delete costs;
}

void CallCount::AddCall(const string& name, 
                        const StringStruct& costs,
                        int   callCount)
{
    auto it = fcalls.find(name);
    if ( it != fcalls.end() ) {
        it->second.calls += callCount;
        it->second += costs;
    } else {
        Calls& count = fcalls[name];
        count.calls = callCount;
        count += costs;
    }
}

std::string CallCount::PrintResults(unsigned tableSize) const {
    stringstream result("");
    static const unsigned name_width = 70;
    const std::string& unitName = NodeConfig::Instance().CostFactory()
                                     .GetName(0);
    result << "-----------------------------------------------------------------------------------------------------------\n";
    result << "|-               Most Time Spent in Function                                                             -|\n";
    result << "-----------------------------------------------------------------------------------------------------------\n";
    result << "|    Function Name                                                      | Calls  | " 
           << left << setw(11) << unitName   << "| " << left << setw(9) << unitName + "/call" << " |\n";
    result << "-----------------------------------------------------------------------------------------------------------\n";
    //           <-          35                   ->  <- 7 ->  <- 11    ->  <- 10   ->
    // Sort into order of most expensive...
    vector<call_pair> mostTotalTime;
    vector<call_pair> mostTimePerCall;

    if ( tableSize == 0 ) {
        tableSize = fcalls.size();
    }

    PopulateTables(tableSize, mostTotalTime,mostTimePerCall);


    // Now print each one...
    for ( const call_pair& it: mostTotalTime ) {
        // Make sure the name doesn't overflow
        string name = it.first;
        if ( name.length() > name_width ) {
            name = name.substr(0,name_width -3) + "...";
        }

        const long& cost = it.second[0];
        result << "| ";
        result << left << setw(name_width) << name;
        result << "| ";
        result << setw(7) << it.second.calls;
        result << "| ";
        result << setw(11) << cost;
        result << "| ";
        result << setw(10) << (it.second.calls == 0 ? 
                                  0 : 
                                  cost / it.second.calls);
        result << "|\n";
    }

    // And complete the table;
    result << "-----------------------------------------------------------------------------------------------------------\n";

    result << "\n";
    result << "-----------------------------------------------------------------------------------------------------------\n";
    result << "|-               Most Expensive Function Calls                                                           -|\n";
    result << "-----------------------------------------------------------------------------------------------------------\n";
    result << "|    Function Name                                                      | Calls  | " 
           << left << setw(11) << unitName   << "| " << left << setw(9) << unitName + "/call" << " |\n";
    result << "-----------------------------------------------------------------------------------------------------------\n";

    // Now print each one...
    for ( const call_pair& it: mostTimePerCall ) {
        // Make sure the name doesn't overflow
        string name = it.first;
        if ( name.length() > name_width ) {
            name = name.substr(0,name_width -3) + "...";
        }

        const long& cost = it.second[0];
        result << "| ";
        result << left << setw(name_width) << name;
        result << "| ";
        result << setw(7) << it.second.calls;
        result << "| ";
        result << setw(11) << cost;
        result << "| ";
        result << setw(10) << (it.second.calls == 0 ?  
                                  0 : 
                                  cost / it.second.calls);
        result << "|\n";
    }

    // And complete the table;
    result << "-----------------------------------------------------------------------------------------------------------\n";
    return result.str();
};

void CallCount::PopulateTables( unsigned tableSize, 
                                vector<call_pair>& mostTotalTime, 
                                vector<call_pair>& mostTimePerCall) const
{
    GetMostTotalTime(mostTotalTime,tableSize);

    mostTimePerCall.resize(tableSize);

    // Select the tableSize most expensive function in terms time per call
    partial_sort_copy(fcalls.begin(),fcalls.end(),
                      mostTimePerCall.begin(),mostTimePerCall.end(),
                      [] (const call_pair& lhs, const call_pair& rhs) -> bool {
                          return ( lhs.second.calls == 0 ? 
                                    0 : 
                                    lhs.second[0] / lhs.second.calls ) >
                                    (rhs.second.calls == 0 ? 
                                     0 : 
                                     rhs.second[0] / rhs.second.calls);
                      });
}


void CallCount::PopulateTables( unsigned tableSize, 
                                vector<call_pair>& mostTotalTime, 
                                vector<call_pair>& mostTimePerCall,
                                const RegPattern& patternRegex) const
{

    vector<call_pair> filteredCalls;
    /*
     * First filter the list...
     */
    GetFilteredSelection(patternRegex, filteredCalls);

    if ( tableSize > filteredCalls.size() ) {
        mostTimePerCall.resize(filteredCalls.size());
    } else {
        mostTimePerCall.resize(tableSize);
    }

    SortByMostTime(filteredCalls,mostTotalTime,tableSize);


    partial_sort_copy(filteredCalls.begin(),filteredCalls.end(),
                      mostTimePerCall.begin(),mostTimePerCall.end(),
                      [=, &patternRegex] (const call_pair& lhs, const call_pair& rhs) -> bool {
                            return ( lhs.second.calls == 0 ? 
                                     0 : 
                                     lhs.second[0] / lhs.second.calls 
                                   ) >
                                   ( rhs.second.calls == 0 ? 
                                     0 : 
                                     rhs.second[0] /rhs.second.calls);
                      });
}

void CallCount::GetFilteredSelection(
    const RegPattern& patternRegex,
    vector<call_pair>& filteredCalls) const
{
    /*
     * First filter the list...
     */
    filteredCalls.reserve(fcalls.size());
    for (const call_pair& call : fcalls) {
        if (patternRegex.Search(call.first)) {
            filteredCalls.push_back(call);
        }
    }
}


void CallCount::PrintWideRow(stringstream& output,
                             const std::string& name, 
                             const int& calls, 
                             const long& usecs) const
{
        output << " " << left << setw(10) << calls;
        output << " " << setw(11) << usecs;
        output << "   " << setw(13) << (calls == 0 ?  
                                          0 : 
                                          usecs / calls);
        output << "  " << name << endl;
}

string CallCount::FilteredPrint(const string& pattern, unsigned tableSize) const {

    stringstream output;
    vector<call_pair> mostTotalTime;
    vector<call_pair> mostTimePerCall;

    if ( tableSize == 0 ) {
        tableSize = fcalls.size();
    }

    try {
        RegPattern patternRegex(pattern);

        // filter and sort the data...
        PopulateTables(tableSize, mostTotalTime,mostTimePerCall, patternRegex);
        PrintTable("Most Time Spent in Function",mostTotalTime,
                                                 false,
                                                 output);
        output << endl << endl;
        PrintTable("Most Expensive Function Calls",mostTimePerCall,
                                              true,
                                              output);
    } catch ( RegError& e ) {
        output << "Invalid regular expression: \n";
        output << e.what();
    }


    return output.str();
}

std::string CallCount::WidePrint(unsigned tableSize) const {
    vector<call_pair> mostTotalTime;
    vector<call_pair> mostTimePerCall;

    if ( tableSize == 0 ) {
        tableSize = fcalls.size();
    }

    PopulateTables(tableSize, mostTotalTime,mostTimePerCall);

    stringstream output;
    PrintTable("Most Time Spent in Function",mostTotalTime,
                                             false,
                                             output);
    output << endl << endl;
    PrintTable("Most Expensive Function Calls",mostTimePerCall,
                                              true,
                                              output);

    return output.str();
}

void CallCount::PrintTable(const string& name, 
                           const vector<call_pair>& rows,
                           bool  average,
                           stringstream& output ) const
{
    output << "                 " << name << endl;
    output << "               ";
    for (size_t i=0; i < name.size() + 4; ++i ) {
        output <<"=";
    }
    output << endl;
    output << " Calls    ";
    string underlines="-------";
    underlines.reserve(90);

    for ( const size_t& i: NodeConfig::Instance().DisplayIdxs() ) {
        const std::string& unitName = NodeConfig::Instance().CostFactory()
                                         .GetName(i);
        if ( average ) {
            output << setw(9) << left << "Av " + unitName << "  ";
        } else {
            output << setw(9) << left << unitName  << "  ";
        }  
        underlines += "  ---------";
    }
    output << "Name" << endl;
    output << underlines << "  -------\n";

    // Now print each one...
    for ( const call_pair& it: rows ) {
        output << left << setw(9) << it.second.calls;
        for ( const size_t& i: NodeConfig::Instance().DisplayIdxs() ) {
            if ( average ) {
                output << setw(11) << 
                    (it.second.calls == 0 ?  
                         0 : 
                         it.second[i] / it.second.calls);
            } else {
                output << setw(11) << it.second[i];
            }
        }
        output << it.first << endl;
    }
}

void CallCount::GetMostTotalTime(
    std::vector<call_pair>& sortedCalls,
    unsigned tableSize) const
{
    if (tableSize > fcalls.size()) {
        tableSize = fcalls.size();
    }

    sortedCalls.resize(tableSize);

    // Select the tableSize most expensive function in terms of total time
    partial_sort_copy(fcalls.begin(),fcalls.end(),
                      sortedCalls.begin(),sortedCalls.end(),
                      [] (const call_pair& lhs, const call_pair& rhs) -> bool {
                          return lhs.second[0] > rhs.second[0];
                      });

}

void CallCount::GetMostTotalTime(
    SortedCallList& sortedCalls,
    const RegPattern& patternRegex,
    unsigned tableSize) const
{
    vector<call_pair> filteredCalls;
    /*
     * First filter the list...
     */
    GetFilteredSelection(patternRegex, filteredCalls);

    SortByMostTime(filteredCalls,sortedCalls,tableSize);
}

void CallCount::SortByMostTime(
    const std::vector<call_pair>& filteredCalls,
    std::vector<call_pair>& mostTotalTime,
    unsigned tableSize) const
{

    if ( tableSize > filteredCalls.size() ) {
        mostTotalTime.resize(filteredCalls.size());
    } else {
        mostTotalTime.resize(tableSize);
    }
    // Select the tableSize most expensive function in terms of total time
    partial_sort_copy(filteredCalls.begin(),filteredCalls.end(),
                      mostTotalTime.begin(),mostTotalTime.end(),
                      [=] (const call_pair& lhs, const call_pair& rhs) -> bool {
                          return lhs.second[0] > rhs.second[0];
                      });
}
