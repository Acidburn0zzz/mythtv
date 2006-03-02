/*
 *  Copyright 2004 - Taylor Jacob (rtjacob at earthlink.net)
 */

#ifndef SIPARSER_H
#define SIPARSER_H

// C includes
#include <cstdio>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include <unistd.h>
#include <sys/types.h>

// C++ includes
#include <iostream>
using namespace std;

// Qt includes
#include <qstringlist.h>
#include <qobject.h>
#include <qstring.h>
#include <qdatetime.h>
#include <qmap.h>
#include <qtextcodec.h>
#include <qmutex.h>

// MythTV includes
#include "sitypes.h"

#ifdef USING_DVB_EIT
#include "eitfixup.h"
#include "eitcache.h"
#else // if !USING_DVB_EIT
typedef void QMap_Events;
#endif // !USING_DVB_EIT

class ProgramAssociationTable;
class ConditionalAccessTable;
class ProgramMapTable;

class ATSCStreamData;
class DVBStreamData;

class MasterGuideTable;
class VirtualChannelTable;
class SystemTimeTable;
class EventInformationTable;
class ExtendedTextTable;

class NetworkInformationTable;
class ServiceDescriptionTable;
class DVBEventInformationTable;

/**
 *  Custom descriptors allow or disallow HUFFMAN_TEXT - For North American 
 *  DVB providers who use Huffman compressed guide in the 9x descriptors.
 */
#define CUSTOM_DESC_HUFFMAN_TEXT               1

/**
 *  Custom descriptors allow or disallow CHANNEL_NUMBERS - For the UK where
 *  channel numbers are sent in one of the SDT tables (at least per scan.c docs)
 */
#define CUSTOM_DESC_CHANNEL_NUMBERS         2

/**
 * The guide source pid.
 */
#define GUIDE_STANDARD                0

/**
 *  GUIDE_DATA_PID is for nonstandard PID being used for EIT style guide
 *  this is seen in North America (this only applies to DVB)
 */
#define GUIDE_DATA_PID                1

/**
 *  Post processing of the guide.  Some carriers put all of the event text
 *  into the description (subtitle, acotors, etc).  You can post processes these
 *  types of carriers EIT data using some simple RegExps to get more powerful
 *  guide data.  BellExpressVu in Canada is one example.
 */
#define GUIDE_POST_PROCESS_EXTENDED        1

class SIParser : public QObject
{
    Q_OBJECT
  public:
    SIParser(const char *name = "SIParser");
    ~SIParser();

    int Start(void);

    // Generic functions that will begin collection of tables based on the
    // SIStandard.
    bool FindTransports(void);
    bool FindServices(void);

    void SetTableStandard(const QString&);
    void SetDesiredProgram(uint mpeg_program_number);

    void ReinitSIParser(const QString &si_std, uint mpeg_program_number);

    // Stops all collection of data and clears all values (on a channel change for example)
    void Reset(void);

    virtual void AddPid(uint pid, uint8_t mask, uint8_t filter,
                        bool CheckCRC, uint bufferFactor) = 0;
    virtual void DelPid(uint pid) = 0;
    virtual void DelAllPids(void) = 0;

    // Functions that may become signals for communication with the outside world
    void ServicesComplete(void);
    void GuideComplete(void);
//    void EventsReady(void);

    // Functions to get objects into other classes for manipulation
    bool GetTransportObject(NITObject &NIT);
    bool GetServiceObject(QMap_SDTObject &SDT);

    void ParseTable(uint8_t* buffer, int size, uint16_t pid);
    void CheckTrackers(void);

    void SetDishNetEIT(bool on)
        { eit_dn_long = on; }

  public slots:
    virtual void deleteLater(void);

    void HandlePAT(const ProgramAssociationTable*);
    void HandleCAT(const ConditionalAccessTable*);
    void HandlePMT(uint pnum, const ProgramMapTable*);
    void HandleMGT(const MasterGuideTable*);
    void HandleSTT(const SystemTimeTable*);
    void HandleVCT(uint pid, const VirtualChannelTable*);
    void HandleEIT(uint pid, const EventInformationTable*);
    void HandleETT(uint pid, const ExtendedTextTable*);
    void HandleNIT(const NetworkInformationTable*);
    void HandleSDT(uint tsid, const ServiceDescriptionTable*);
    void HandleEIT(const DVBEventInformationTable*);

  signals:
    void FindTransportsComplete(void);
    void FindServicesComplete(void);
    void FindEventsComplete(void);
    void TableLoaded(void);
    void UpdatePMT(const ProgramMapTable *pmt);
    void EventsReady(QMap_Events* Events);
    void AllEventsPulled(void);

  protected:
    void CountUnusedDescriptors(uint pid, const unsigned char *data);
    void PrintDescriptorStatistics(void) const;

  private:
    uint GetLanguagePriority(const QString &language);

    // Fixes for various DVB Network Spec Deviations
    void LoadPrivateTypes(uint networkID);

    // DVB Descriptor Parsers
    void HandleNITDesc(const desc_list_t &dlist);
    void HandleNITTransportDesc(const desc_list_t &dlist,
                                TransportObject   &tobj,
                                QMap_uint16_t     &clist);

#ifdef USING_DVB_EIT
    // DVB EIT Table Descriptor processors
    uint ProcessDVBEventDescriptors(
        uint                         pid,
        const unsigned char          *data,
        uint                         &bestPrioritySE,
        const unsigned char*         &bestDescriptorSE, 
        uint                         &bestPriorityEE,
        vector<const unsigned char*> &bestDescriptorsEE,
        Event                        &event);

#endif //USING_DVB_EIT

  private:
    // Timeout Variables
    QDateTime TransportSearchEndTime;
    QDateTime ServiceSearchEndTime;
    QDateTime EventSearchEndTime;

    // Common Variables
    int                 table_standard;
    uint                CurrentTransport;
    uint                RequestedServiceID;
    uint                RequestedTransportID;
    
    // Preferred languages and their priority
    QMap<QString,uint>  LanguagePriority;

    // DVB Variables
    uint                NITPID;

    // Storage Objects (DVB)
    NITObject           NITList;

    // Storage Objects (ATSC)
    QMap<uint,uint>     sourceid_to_channel;
    ATSCStreamData     *atsc_stream_data;
    DVBStreamData      *dvb_stream_data;

    // Mutex Locks
    // TODO: Lock Events, and Services, Transports, etc
    QMutex              pmap_lock;
    pnum_pid_map_t      pnum_pid;
    dvb_srv_eit_on_t    dvb_srv_collect_eit;
    atsc_eit_pid_map_t  atsc_eit_pid;
    atsc_ett_pid_map_t  atsc_ett_pid;

    int                 ThreadRunning;
    bool                exitParserThread;
    TableSourcePIDObject TableSourcePIDs;
    bool                standardChange;
    /// Decode DishNet's long-term DVB EIT
    bool                eit_dn_long;

    // New tracking objects
    TableHandler       *Table[NumHandlers+1];
    privateTypes        PrivateTypes;
    bool                PrivateTypesLoaded;

#ifdef USING_DVB_EIT
    /// EITFixUp instance
    EITFixUp            eitfixup;
    EITCache            eitcache;
    QMap2D_Events       incomplete_events;
    QMap2D_Events       complete_events;
#endif

    // statistics
    QMap<uint,uint>     descCount;
    mutable QMutex      descLock;
};

#endif // SIPARSER_H

