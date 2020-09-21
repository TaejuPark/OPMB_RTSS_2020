#ifndef __ROUTING_TABLE_H__
#define __ROUTING_TABLE_H__

// Routing table at PDUR Module

using namespace std;

enum routing_type
{
    PDU_DIRECT,
    SIGNAL,
};

enum iotype
{
    SEND,
    RECEIVE,
};

struct pdurt
{
    int pduid; // PDU ID
    int sbus;  // Source bus
    vector<int> dbus; // Destination buses
    enum routing_type rtype; // Routing type
};

// Routing table at COM Module
struct comrt
{
    int pduid; // PDU ID
    enum iotype iotype; // Rx PDU or Tx PDU
    vector<struct signal> signals; // signals in the PDU
    vector<int> dbus; // Destination buses
};

// RxPDU-TxPDU mapping table for each signal
struct sigmap
{
    int sigid; // Signal ID
    int rxpduid; // Rx PDU for the signal
    vector<int> txpduid; // Tx PDUs for the signal
};

struct routing_table
{
    vector<struct pdurt> pdurt;
    vector<struct comrt> comrt;
};

#endif
