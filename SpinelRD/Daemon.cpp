//===========================================================================
#include "stdafx.h"
#include <tlhelp32.h>
#include <shellApi.h>
#include <subauth.h>
#include <process.h>
#include <iterator>
#include <cstring>
#include <string>
#include <vector>
#include <set>
#include "pryutil.h"
#include "SpinelRD.h"
#include "Daemon.h"
//---------------------------------------------------------------------------

using namespace std;

//===========================================================================
// Functions
//---------------------------------------------------------------------------
static __int64 GetDiskFreeSpaceFromFileName(string FileName)
{
  string Drive = file_drive_of(FileName);

  DWORD SectorsPerCluster;
  DWORD BytesPerSector;
  DWORD FreeClusters;
  DWORD Clusters;

  BOOL Result=GetDiskFreeSpaceA(
    Drive.c_str(),
    &SectorsPerCluster,
    &BytesPerSector,
    &FreeClusters,
    &Clusters
  );

  if(Result)
    return __int64(FreeClusters)*
           __int64(SectorsPerCluster)*
           __int64(BytesPerSector)       ;

  return -1;
}
//---------------------------------------------------------------------------
static string IntToKMGT(__int64 val)
{
  double bytes=(double)val;
  bool neg = bytes<0 ;
  if(neg) bytes = -bytes ;
  int unitno=-1;
  while(bytes>1024) {
    bytes/=1024; unitno++;
  }
  string text;
  if(unitno<0)
    text = itos((int)val);
  else
    text = str_printf("%5.2f",bytes);
  if(unitno>=0) {
    switch(unitno) {
      case 0: text=text+"K"; break;
      case 1: text=text+"M"; break;
      case 2: text=text+"G"; break;
      case 3: text=text+"T"; break;
      default: text=text+"?"; break;
    }
  }
  return (neg?"-":"")+text;
}
//===========================================================================
// �Ď��^�C�}�[�N���X CDaemonTimer
//---------------------------------------------------------------------------
static UINT DaemonTimer_StaticId=0;
static MAP_DaemonTimer DaemonTimerMap ;
//---------------------------------------------------------------------------
CDaemonTimer::CDaemonTimer(HWND hwnd, EVENT_DaemonTimer event, void *arg,
  int interval, bool enabled, int id)
 : hwnd_(hwnd), enabled_(enabled), id_(id?id:++DaemonTimer_StaticId),
   interval_(interval), event_(event), arg_(arg)
{
  DaemonTimerMap[id_] = this ;
  Update();
}
//---------------------------------------------------------------------------
CDaemonTimer::~CDaemonTimer()
{
  KillTimer(hwnd_, id_);
  DaemonTimerMap.erase(id_);
}
//---------------------------------------------------------------------------
VOID CALLBACK CDaemonTimer::TimerProc(HWND hwnd, UINT uMsg,
    UINT idEvent, DWORD dwTime)
{
  MAP_DaemonTimer::iterator pos = DaemonTimerMap.find(idEvent);
  if(pos!=DaemonTimerMap.end())
    pos->second->DoEvent();
}
//---------------------------------------------------------------------------
void CDaemonTimer::Update()
{
  KillTimer(hwnd_, id_);
  if(enabled_) SetTimer(hwnd_, id_, interval_, &TimerProc);
}
//===========================================================================
// ���C���Ď��N���X CMainDaemon
//---------------------------------------------------------------------------
CMainDaemon MainDaemon;

#define THREAD_TERMINATE_LIMIT    30000
#define DEFMAXBYTES               9223372036854775807

#define DEFDAEMONSHOWTASKICON     0
#define DEFDAEMONJOBPAUSE         0
#define DEFDAEMONLOGENABLED       0
#define DEFDAEMONJOBINTERVAL      60
#define DEFDAEMONTHREADPRIORITY   THREAD_PRIORITY_LOWEST
#define DEFSPINELENABLED          1
#define DEFSPINELEXENAME          "Spinel.exe"
#define DEFSPINELEXEPARAM         "-WindowState=Minimized"
#define DEFSPINELDEATHRESUME      0
#define DEFSPINELPROCESSPRIORITY  0
#define DEFSPINELREMOVEUISETTING  1
#define DEFSPINELRESUMEDELAY      5000
#define DEFLOGROTATIONENABLED     1
#define DEFLOGROTATIONMAXFILES    INT_MAX     //Log�f�B���N�g���Ɏc���ő働�O�t�@�C����
#define DEFLOGROTATIONMAXBYTES    DEFMAXBYTES //Log�f�B���N�g���̍ő勖�e�o�C�g��

//---------------------------------------------------------------------------
CMainDaemon::CMainDaemon()
  : HInstance(NULL), HWMain(NULL), first(true), niEntried(false),
    StartTimer(NULL), ResumeTimer(NULL), JobTimer(NULL)
{
  Suspended = Resumed = EndSession = Finalized = JobAborted = false ;
  JobThread = INVALID_HANDLE_VALUE ;
}
//---------------------------------------------------------------------------
void CMainDaemon::Initialize(HINSTANCE InstanceHandle, HWND MainWindowHandle)
{
  HInstance = InstanceHandle ;
  HWMain = MainWindowHandle ;

  StartTimer = new CDaemonTimer(HWMain,&DaemonTimerEventCallback,this,1);
  ResumeTimer = new CDaemonTimer(HWMain,&DaemonTimerEventCallback,this,DEFSPINELRESUMEDELAY,false);
  JobTimer = new CDaemonTimer(HWMain,&DaemonTimerEventCallback,this,DEFDAEMONJOBINTERVAL*60*1000,false);

  DAEMONShowTaskIcon    = DEFDAEMONSHOWTASKICON    ;
  DAEMONLogEnabled      = DEFDAEMONLOGENABLED      ;
  DAEMONJobPause        = DEFDAEMONJOBPAUSE        ;
  DAEMONThreadPriority  = DEFDAEMONTHREADPRIORITY  ;
  SpinelEnabled         = DEFSPINELENABLED         ;
  SpinelExeName         = DEFSPINELEXENAME         ;
  SpinelExeParam        = DEFSPINELEXEPARAM        ;
  SpinelDeathResume     = DEFSPINELDEATHRESUME     ;
  SpinelProcessPriority = DEFSPINELPROCESSPRIORITY ;
  SpinelProcess         = NULL                     ;
  SpinelRemoveUISetting = DEFSPINELREMOVEUISETTING ;
  LogRotationEnabled    = DEFLOGROTATIONENABLED    ;
  LogRotationMaxFiles   = DEFLOGROTATIONMAXFILES   ;
  LogRotationMaxBytes   = DEFLOGROTATIONMAXBYTES   ;
  IniLastAge            = 0                        ;
}
//---------------------------------------------------------------------------
void CMainDaemon::Finalize()
{
  if(!Finalized) {
    Finalized=true;
    TerminateJob() ;
    DeleteTaskIcon();
    if(JobTimer) { delete JobTimer ; JobTimer = NULL ; }
    if(ResumeTimer) { delete ResumeTimer ; ResumeTimer = NULL ; }
    if(StartTimer) { delete StartTimer ; StartTimer = NULL ; }
    if(SpinelProcess) {
      CloseHandle(SpinelProcess);
      SpinelProcess=NULL;
    }
    LogOut("<<" SPINELRD_VER " Exit>>\n");
  }
}
//---------------------------------------------------------------------------
string CMainDaemon::AppExeName()
{
  char szPath[_MAX_PATH] ;
  GetModuleFileNameA( HInstance, szPath, _MAX_PATH ) ;
  return szPath ;
}
//---------------------------------------------------------------------------
void CMainDaemon::LoadIni()
{
  string iniFileName = file_path_of(AppExeName()) + file_prefix_of(AppExeName()) + ".ini" ;
  if(!file_is_existed(iniFileName)) {
    DBGOUT("Ini file \"%s\" was not existed.\n",iniFileName.c_str());
    return ;
  }

  // TODO: FindFirstChangeNotificationA
  int age = file_age_of(iniFileName) ;
  if(age==IniLastAge) return ;

  if(!first)
    LogOut("ini�t�@�C�� \"%s\" �̕ύX�����m���܂����B",iniFileName.c_str()) ;

  const DWORD BUFFER_SIZE = 1024 ;
  char buffer[BUFFER_SIZE] ;
  ZeroMemory(buffer,BUFFER_SIZE) ;
  string Section ;

  #define LOADSTR2(val,key) do { \
      GetPrivateProfileStringA(Section.c_str(),key,val.c_str(),buffer,BUFFER_SIZE,iniFileName.c_str()) ; \
      val = buffer ; \
    }while(0)
  #define LOADSTR(val) LOADSTR2(val,#val)
  #define LOADWSTR(val) do { \
      string temp = wcs2mbcs(val) ; \
      LOADSTR2(temp,#val) ; val = mbcs2wcs(temp) ; \
    }while(0)

  #define LOADINT2(val,key,a2i) do { \
      GetPrivateProfileStringA(Section.c_str(),key,"",buffer,BUFFER_SIZE,iniFileName.c_str()) ; \
      val = a2i(buffer,val) ; \
    }while(0)
  #define LOADINT(val) LOADINT2(val,#val,acalci)
  #define LOADINT64(val) LOADINT2(val,#val,acalci64)

  #define LOADSTR_SEC(sec,val) do {\
      Section = #sec ; \
      LOADSTR2(sec##val,#val); \
    }while(0)
  #define LOADINT_SEC(sec,val) do {\
      Section = #sec ; \
      LOADINT2(sec##val,#val,acalci); \
    }while(0)
  #define LOADINT64_SEC(sec,val) do {\
      Section = #sec ; \
      LOADINT2(sec##val,#val,acalci64); \
    }while(0)

    int DAEMONJobInterval = JobTimer->Interval()/(60*1000) ;
    int SpinelResumeDelay = ResumeTimer->Interval() ;

    LOADINT_SEC(DAEMON,ShowTaskIcon) ;
    LOADINT_SEC(DAEMON,LogEnabled) ;
    LOADINT_SEC(DAEMON,JobPause) ;
    LOADINT_SEC(DAEMON,JobInterval) ;
    LOADINT_SEC(DAEMON,ThreadPriority) ;

    if(first)
      LogOut("<<" SPINELRD_VER " Start>>");

    LogOut("-*- ini�t�@�C�� \"%s\" �̃����[�h���J�n���܂����B-*-",iniFileName.c_str()) ;

    LogOut(" [DAEMON]");
    LogOut("\t�^�X�N�A�C�R���̕\��:\t%s", DAEMONShowTaskIcon?"�L��":"����");
    LogOut("\t���O�̏o��:\t%s", DAEMONLogEnabled?"�L��":"����");
    LogOut("\t�W���u�̒��f:\t%s", DAEMONJobPause?"�L��":"����");
    LogOut("\t�W���u�̎��s�Ԋu:\t%d���u���Ɉ��", DAEMONJobInterval);
    string StrThPrior="�s��";
    switch(DAEMONThreadPriority) {
    case THREAD_PRIORITY_IDLE:
        StrThPrior = "�A�C�h�����̂�" ; break;
    case THREAD_PRIORITY_LOWEST:
        StrThPrior = "�Ⴂ" ; break;
    case THREAD_PRIORITY_BELOW_NORMAL:
        StrThPrior = "���Ⴂ" ; break;
    case THREAD_PRIORITY_NORMAL:
        StrThPrior = "�ʏ�" ; break;
    case THREAD_PRIORITY_ABOVE_NORMAL:
        StrThPrior = "��⍂��" ; break;
    case THREAD_PRIORITY_HIGHEST:
        StrThPrior = "����" ; break;
    case THREAD_PRIORITY_TIME_CRITICAL:
        StrThPrior = "���A���^�C��" ; break;
    }
    LogOut("\t�W���u�X���b�h�̗D��x:\t%s(%d)", StrThPrior.c_str(), DAEMONThreadPriority);

    LOADINT_SEC(Spinel,Enabled) ;
    LOADSTR_SEC(Spinel,ExeName) ;
    LOADSTR_SEC(Spinel,ExeParam) ;
    LOADINT_SEC(Spinel,DeathResume) ;
    LOADINT_SEC(Spinel,ProcessPriority) ;
    LOADINT_SEC(Spinel,RemoveUISetting) ;
    LOADINT_SEC(Spinel,ResumeDelay) ;

    LogOut(" [Spinel]");
    LogOut("\t�����Ď�:\t%s", SpinelEnabled?"�L��":"����");
    LogOut("\t�����Ď�����v���O����:\t\"%s\"", SpinelExeName.c_str());
    LogOut("\t�����Ď�����v���O�����̈���:\t%s",SpinelExeParam.c_str()) ;
    LogOut("\t�펞�����Ď�:\t%s", SpinelDeathResume?"�L��":"����");
    string StrSpPrior="�s��";
    switch(SpinelProcessPriority) {
    case REALTIME_PRIORITY_CLASS:
        StrSpPrior = "���A���^�C��" ; break;
    case ABOVE_NORMAL_PRIORITY_CLASS:
        StrSpPrior = "����" ; break;
    case NORMAL_PRIORITY_CLASS:
        StrSpPrior = "�ʏ�" ; break;
    case BELOW_NORMAL_PRIORITY_CLASS:
        StrSpPrior = "�Ⴂ" ; break;
    case IDLE_PRIORITY_CLASS:
        StrSpPrior = "�A�C�h�����̂�" ; break;
    case 0:
        StrSpPrior = "���ύX" ; break;
    }
    LogOut("\t�����Ď��v���Z�X�̗D��x:\t%s(%d)", StrSpPrior.c_str(), SpinelProcessPriority);
    LogOut("\t�N����UI�ݒ�t�@�C��(UISetting.json)�̖���:\t%s", SpinelRemoveUISetting?"�L��":"����");
    LogOut("\t�����Ď��Ԋu:\t%dmsec", SpinelResumeDelay);

    LOADINT_SEC(LogRotation,Enabled) ;
    LOADINT_SEC(LogRotation,MaxFiles) ;
    LOADINT64_SEC(LogRotation,MaxBytes) ;

    LogOut(" [LogRotation]");
    LogOut("\t���O���[�e�[�V�����@�\:\t%s", LogRotationEnabled?"�L��":"����");
    LogOut("\t���O�ő�t�@�C����:\t%s", (
        LogRotationMaxFiles==INT_MAX?string("�����i�V"):
        str_printf("%d",LogRotationMaxFiles)).c_str() );
    LogOut("\t���O�ő�o�C�g��:\t%s", (
        LogRotationMaxBytes==DEFMAXBYTES?string("�����i�V"):
        IntToKMGT(LogRotationMaxBytes)).c_str() );

    JobTimer->SetInterval(DAEMONJobInterval*60*1000) ;
    JobTimer->SetEnabled(!DAEMONJobPause);
    ResumeTimer->SetInterval(SpinelResumeDelay) ;

    if(DAEMONJobPause)
      TerminateJob();

    if(!first) {
      if(DAEMONShowTaskIcon)
        AddTaskIcon();
      else
        DeleteTaskIcon();
    }

    if(SpinelProcess&&SpinelProcessPriority)
      SetPriorityClass(SpinelProcess,SpinelProcessPriority) ;

    // Load User Rotations

    const string RotationPrefix = "Rotation." ;
    vector<string> sections;
    {
      BUFFER<char> buf(256) ;
      DWORD n = 0;
      do {
        if (n) buf.resize(buf.size()*2) ;
        n = GetPrivateProfileSectionNamesA(buf.data(), (DWORD)buf.size(), iniFileName.c_str());
      } while (n == buf.size() - 2);
      if( n ) {
        for (size_t i = 0;i < n;i++) {
          char *p = buf.data() ;
          p = &p[i] ;
          if (!*p)
            break ;
          if(!_strnicmp(p,RotationPrefix.c_str(),RotationPrefix.size()))
            sections.push_back(p);
          i += strlen(p);
        }
      }
    }

    TRotations rotations ;

    for(size_t i=0;i<sections.size();i++) {
      Section = sections[i] ;
      LogOut(" [%s]",Section.c_str());
      string FileMask;
      LOADSTR(FileMask) ;
      if(FileMask!="") {
        LogOut("\t�t�@�C���}�X�N:\t%s", FileMask.c_str());
        BOOL Enabled=TRUE ;
        int MaxFiles=INT_MAX, MaxDays=INT_MAX ;
        __int64 MaxBytes=DEFMAXBYTES ;
        BOOL SubDirectories=0 ;
        string FellowSuffix="^";
        LOADINT(Enabled);
        LogOut("\t���[�e�[�V�����@�\:\t%s", Enabled?"�L��":"����(�X�L�b�v���܂��B)");
        if(!Enabled) continue ;
        LOADINT(MaxFiles);
        LogOut("\t�ő�t�@�C����:\t%s", (
            MaxFiles==INT_MAX?string("�����i�V"):
            str_printf("%d",MaxFiles)).c_str() );
        LOADINT(MaxDays);
        LogOut("\t�ő�ێ�����:\t%s", (
            MaxDays==INT_MAX?string("�����i�V"):
            str_printf("%d����",MaxDays)).c_str() );
        LOADINT64(MaxBytes);
        LogOut("\t�ő�o�C�g��:\t%s", (
            MaxBytes==DEFMAXBYTES?string("�����i�V"):
            IntToKMGT(MaxBytes)).c_str() );
        LOADINT(SubDirectories);
        LogOut("\t�T�u�f�B���N�g��:\t%s", SubDirectories?"�L��":"����");
        LOADSTR(FellowSuffix) ;
        rotations.push_back(
          TRotationItem(FileMask,MaxFiles,MaxDays,MaxBytes,!!SubDirectories,FellowSuffix));
        LogOut("\t�A���폜�T�t�B�b�N�X:\t%s", FellowSuffix=="^"?"<�Ȃ�>":FellowSuffix.c_str());
      }
    }

    if(LogRotationEnabled) {
      string LogPath = file_path_of(AppExeName())+"Log\\" ;
      string Mask = LogPath+"*.txt" ;
      rotations.push_back(
        TRotationItem(Mask,LogRotationMaxFiles,INT_MAX,LogRotationMaxBytes) );
    }

    struct a_p_splitter : public unary_function<TRotationItem,bool> {
      bool operator ()(const TRotationItem &v) const {
        return v.MaxBytes>=0; // true: active(front), false: passive(back)
      }
    };

    partition(rotations.begin(),rotations.end(),a_p_splitter()) ;

    UserRotations.swap(rotations) ;

    IniLastAge = age ;

    LogOut("-*- ini�t�@�C�� \"%s\" �̃����[�h���������܂����B-*-",iniFileName.c_str()) ;

  #undef LOADINT64_SEC
  #undef LOADINT_SEC
  #undef LOADSTR_SEC

  #undef LOADINT64
  #undef LOADINT
  #undef LOADINT2

  #undef LOADSTR
  #undef LOADWSTR
  #undef LOADSTR2
}
//---------------------------------------------------------------------------
void CMainDaemon::LogOut(const char* format, ...)
{
    va_list marker ;
    va_start( marker, format ) ;
    int edit_ln = _vscprintf(format, marker);
    if(edit_ln++>0) {
        SYSTEMTIME tm ;
        GetLocalTime(&tm);
        string out_str = str_printf("[%d/%02d/%02d %02d:%02d:%02d.%03d] ",
            tm.wYear, tm.wMonth, tm.wDay,
            tm.wHour, tm.wMinute, tm.wSecond, tm.wMilliseconds );
        {
            BUFFER<char> edit_str(edit_ln);
            vsprintf_s( edit_str.data(), edit_ln, format, marker ) ;
            out_str += string(edit_str.data()) + '\n' ;
        }
        exclusive_lock lock(&exclLog);
        OutputDebugStringA(out_str.c_str()) ;
        if(DAEMONLogEnabled) {
            string log_dir = file_path_of(AppExeName())+"Log" ;
            if(!folder_is_existed(log_dir))
                CreateDirectoryA(log_dir.c_str(),NULL) ;
            string log_file = log_dir+"\\daemonlog" ;
            log_file += str_printf("%d%02d%02d.txt",tm.wYear,tm.wMonth,tm.wDay);
            FILE *fp = NULL ;
            fopen_s(&fp,log_file.c_str(),"a+t") ;
            if(fp) {
                fputs(out_str.c_str(),fp) ;
                fclose(fp) ;
            }
        }
    }
    va_end( marker ) ;
}
//---------------------------------------------------------------------------

  typedef std::vector<PROCESSENTRY32> TProcessEntries ;
  static int EnumExecuting(string ExeName,TProcessEntries *OutEntries=NULL) {
    HANDLE Process = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0) ;
    PROCESSENTRY32 Entry ;
    ZeroMemory(&Entry,sizeof(Entry)) ;
    Entry.dwSize = sizeof(Entry) ;
    int NumResult=0 ;
    if(Process32First(Process,&Entry)) {
      do {
        if(!_stricmp(wcs2mbcs(Entry.szExeFile).c_str(),ExeName.c_str())) {
          if(OutEntries) OutEntries->push_back(Entry) ;
          NumResult++;
        }
      } while(Process32Next(Process,&Entry)) ;
    }
    CloseHandle(Process) ;
    return NumResult ;
  }

  static int FullExecutingEntryOf(string FullExeName , TProcessEntries *OutEntries=NULL) {
    string prefix = file_name_of(FullExeName) ;
    TProcessEntries enumerates ;
    int NumResult=0 ;
    if(EnumExecuting(prefix,&enumerates)>0) {
      for(size_t i=0;i<enumerates.size();i++) {
        HANDLE Process = OpenProcess(
          PROCESS_QUERY_LIMITED_INFORMATION,FALSE,
          enumerates[i].th32ProcessID );
        if(Process!=NULL) {
          BUFFER<char> buf(MAX_PATH) ;
          DWORD path_ln = buf.size() ;
          if(QueryFullProcessImageNameA(
              Process,0,buf.data(),&path_ln)) {
            if(!_stricmp(buf.data(),FullExeName.c_str())) {
              if(OutEntries) OutEntries->push_back(enumerates[i]);
              NumResult++;
            }
          }
          CloseHandle(Process);
        }
      }
    }
    return NumResult ;
  }

//---------------------------------------------------------------------------
void CMainDaemon::RemoveUISetting()
{
  string FileName = file_path_of(AppExeName())+"Config\\UISetting.json" ;
  if(file_is_existed(FileName)) {
    if(DeleteFileA(FileName.c_str())) {
      LogOut("- UI�ݒ�t�@�C�� \"%s\" ���폜���܂����B",FileName.c_str());
      Sleep(1000);
    }
  }
}
//---------------------------------------------------------------------------
bool CMainDaemon::LaunchSpinel()
{
    if(!SpinelEnabled) return true ;
    string FileName = file_path_of(AppExeName())+SpinelExeName ;
    string WorkDir = file_path_of(FileName)+"." ;
    string Parameters =  SpinelExeParam ;

    TProcessEntries Entries ;
    if(FullExecutingEntryOf(FileName,&Entries)>0) {
      if(SpinelDeathResume&&SpinelProcess==NULL) {
        SpinelProcess = OpenProcess(
          PROCESS_SET_INFORMATION |
          PROCESS_QUERY_LIMITED_INFORMATION |
          PROCESS_TERMINATE |
          SYNCHRONIZE,
          FALSE,Entries[0].th32ProcessID );
        if(SpinelProcess) {
          if(SpinelProcessPriority)
            SetPriorityClass(SpinelProcess,SpinelProcessPriority) ;
          LogOut("�Ď��Ώۃv���Z�X [\"%s\"] �ɃA�^�b�`���܂����B",FileName.c_str());
        }
      }
      return true ;
    }

    if(SpinelRemoveUISetting) RemoveUISetting();

      SHELLEXECUTEINFOA info ;
      ZeroMemory(&info,sizeof(info));
      info.cbSize = sizeof(SHELLEXECUTEINFO) ;
      info.fMask |= SEE_MASK_NOCLOSEPROCESS ;
      info.hwnd = HWMain ;
      info.lpVerb = NULL ;
      info.lpFile = FileName.c_str();
      info.lpParameters = Parameters.c_str() ;
      info.lpDirectory = WorkDir.c_str() ;
      info.nShow = SW_SHOWMINIMIZED;
      info.hInstApp = 0 ;
      info.lpIDList = NULL;
      info.lpClass = NULL ;
      info.hkeyClass = 0 ;
      info.dwHotKey = 0;
      info.hIcon = 0;
      info.hProcess = 0 ;
      if(ShellExecuteExA(&info)) {
        if(SpinelProcess) {
          CloseHandle(SpinelProcess) ;
          SpinelProcess=NULL ;
        }
        if(info.hProcess) {
          if(!WaitForInputIdle(info.hProcess,ResumeTimer->Interval())) {
            if(SpinelProcessPriority)
              SetPriorityClass(info.hProcess,SpinelProcessPriority) ;
          }else {
            TerminateProcess(info.hProcess,-9) ;
            CloseHandle(info.hProcess) ;
            return false ;
          }
          if(SpinelDeathResume) {
            SpinelProcess = info.hProcess ;
          }else {
            CloseHandle(info.hProcess) ;
          }
        }
        if(!Parameters.empty())
          LogOut("�Ď��Ώۃv���Z�X [\"%s\" %s] ���N�����܂����B",FileName.c_str(),Parameters.c_str());
        else
          LogOut("�Ď��Ώۃv���Z�X [\"%s\"] ���N�����܂����B",FileName.c_str());
        return true ;
      }

   return false ;
}
//---------------------------------------------------------------------------

  struct logdata {
    string fname ;
    DWORD ftime ;
    __int64 fsize ;
    int tag ;
    logdata(const WIN32_FIND_DATAA &find_data, int tag_=-1,const string &relPath="") : tag(tag_) {
      FILETIME local ;
      WORD d=0, t=0 ;
      fname = relPath + string(find_data.cFileName) ;
      FileTimeToLocalFileTime(&find_data.ftLastWriteTime, &local);
      FileTimeToDosDateTime(&local, &d, &t);
      ftime = DWORD(d)<<16|DWORD(t) ;
      fsize = __int64(find_data.nFileSizeHigh)<<32 | __int64(find_data.nFileSizeLow) ;
    }
  };

  struct logcomp : public binary_function<bool,logdata,logdata> {
    bool operator ()(const logdata &lhs,const logdata &rhs) const {
      return lhs.ftime<rhs.ftime ; // �Â��t�@�C����擪��
    }
  };

  typedef vector<string> dirs_t ;

  static int enum_dirs(dirs_t &dirs,const string &Path,bool *pAborted=NULL) {
    int n = 0 ;
    WIN32_FIND_DATAA Data ;
    HANDLE HFind = FindFirstFileA((Path+"*.*").c_str(),&Data) ;
    if(HFind!=INVALID_HANDLE_VALUE) {
        do {
            if(pAborted&&*pAborted) break ;
            if(!(Data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)) continue ;
            if(Data.cFileName&&Data.cFileName[0]!='.') {
                dirs.push_back(Data.cFileName) ;
                n++;
            }
        }while(FindNextFileA(HFind,&Data)) ;
        FindClose(HFind) ;
    }
    return n ;
  }

  typedef multiset<logdata,logcomp> logset ;

  static __int64 make_logset(logset &LogSet,
      const string &FilePath,const masks_t &Masks,bool SubDirec=false,
      DWORD *pMaxTime=NULL,bool *pAborted=NULL, int tag=-1,const string &relPath="") {

    WIN32_FIND_DATAA Data ;

    __int64 LogBytes=0 ;
    DWORD maxTime = 0 ;

    for(size_t i=0;i<Masks.size();i++) {
      string LogPathMask = FilePath+relPath+Masks[i] ;
      HANDLE HFind = FindFirstFileA(LogPathMask.c_str(),&Data) ;
      if(HFind!=INVALID_HANDLE_VALUE) {
          do {
              if(pAborted&&*pAborted) break ;
              if(Data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) continue ;
              if(Data.dwFileAttributes&FILE_ATTRIBUTE_READONLY) continue ;
              logdata ldata(Data,tag,relPath) ;
              LogSet.insert(ldata);
              LogBytes += ldata.fsize ;
              if( ldata.ftime > maxTime ) maxTime = ldata.ftime ;
          }while(FindNextFileA(HFind,&Data)) ;
          FindClose(HFind) ;
      }
    }

    if(SubDirec) {
      dirs_t dirs ;
      if(enum_dirs(dirs,FilePath+relPath,pAborted)>0) {
        for(size_t i=0;i<dirs.size();i++) {
          if(pAborted&&*pAborted) break ;
          DWORD subMaxTime=0;
          LogBytes+=make_logset(
            LogSet,FilePath,Masks,true,&subMaxTime,pAborted,tag,relPath+dirs[i]+"\\");
          if(subMaxTime>maxTime) maxTime = subMaxTime ;
        }
      }
    }

    if(pMaxTime) *pMaxTime = maxTime ;
    return LogBytes ;
  }

static void LogRotateFiles(CMainDaemon *daemon,
    const string &LogPath,const masks_t &Masks,int MaxFiles,int MaxDays,
    __int64 MaxFileBytes,bool SubDirectories,const masks_t &FellowMasks, bool &Aborted )
{
  logset LogSet ;
  DWORD maxTime = 0 ;
  __int64 LogBytes= make_logset(LogSet,LogPath,Masks,SubDirectories,&maxTime,&Aborted) ;

  if(!FellowMasks.empty()) {
    set<string> done ;
    for(logset::iterator pos=LogSet.begin();pos!=LogSet.end();++pos) {
      if(Aborted) break ;
      done.insert(lower_case(LogPath + pos->fname));
    }
    for(logset::iterator pos=LogSet.begin();pos!=LogSet.end();++pos) {
      if(Aborted) break ;
      string FileName = LogPath + pos->fname ;
      string fpath = lower_case( file_path_of(FileName) ) ;
      string fpathprefix = fpath + lower_case( file_prefix_of(FileName) );
      logset fellow_logset ;
      make_logset(fellow_logset,fpathprefix,FellowMasks) ;
      for( logset::iterator flpos = fellow_logset.begin() ;
           flpos != fellow_logset.end() ; ++flpos ) {
        string fellow_filename = fpath + lower_case(flpos->fname) ;
        if(!done.count(fellow_filename)) {
          LogBytes += flpos->fsize ;
          done.insert(fellow_filename) ;
        }
      }
    }
  }

  if(Aborted) return ;

  __int64 BorderBytes ;
  if(MaxFileBytes>0) BorderBytes = MaxFileBytes ;
  else {
    __int64 FreeSpace = GetDiskFreeSpaceFromFileName(LogPath) ;
    __int64 NeedSpace = -MaxFileBytes ;
    if(FreeSpace>=0&&NeedSpace>FreeSpace) {
      __int64 Space = NeedSpace-FreeSpace ;
      BorderBytes = LogBytes - Space ; // �󂫂ɗ]�T���Ȃ���������������
    }else {
      BorderBytes = LogBytes ; // �󂫂ɗ]�T�����邩�f�B�X�N�T�C�Y���ʕs�\
    }
  }

  DWORD BorderTime ;
  DWORD maxDays = DWORD(MaxDays)<<16 ;
  if(maxTime<maxDays) BorderTime=0 ;
  else BorderTime = maxTime - maxDays ;

  int LogFiles = LogSet.size() ;
  for(logset::iterator pos=LogSet.begin();pos!=LogSet.end();++pos) {
    if(Aborted) break ;
    if(LogFiles<=MaxFiles&&LogBytes<=BorderBytes&&pos->ftime>=BorderTime) break ;
    string FileName = LogPath + pos->fname ;
    bool existed = file_is_existed(FileName) ;
    if(!existed||DeleteFileA(FileName.c_str())) {
      if(existed)
        daemon->LogOut("- �t�@�C�� \"%s\" ���폜���܂����B",FileName.c_str());
      if(existed) LogBytes -= pos->fsize ; LogFiles-- ;
      logset fellow_logset ;
      string fpath = file_path_of(FileName) ;
      string fpathprefix = fpath + file_prefix_of(FileName) ;
      make_logset(fellow_logset,fpathprefix,FellowMasks) ;
      for( logset::iterator flpos = fellow_logset.begin();
           flpos != fellow_logset.end(); flpos++ ) {
        string fellow_filename = fpath+flpos->fname ;
        if(DeleteFileA(fellow_filename.c_str())) {
          daemon->LogOut(" - �t�@�C�� \"%s\" ���폜���܂����B"
            ,fellow_filename.c_str());
          LogBytes -= flpos->fsize ;
        }
      }
    }
  }

}


//---------------------------------------------------------------------------
void CMainDaemon::JobRotate()
{
  size_t i;
  for(i=0;i<JobRotations.size();i++) { // Active Rotation (MaxBytes limited)
    if(JobRotations[i].MaxBytes<0) break ; // passive found
    LogRotateFiles(this,
      JobRotations[i].FilePath,
      JobRotations[i].Masks,
      JobRotations[i].MaxFiles,
      JobRotations[i].MaxDays,
      JobRotations[i].MaxBytes,
      JobRotations[i].SubDirectories,
      JobRotations[i].FellowMasks,JobAborted ) ;
    if(JobAborted) return ;
  }

  if(i<JobRotations.size()) { // Passive Rotation (Depends on disk free space)

    map<string/*drv*/,__int64/*free*/> spaces ;

    struct TStat {
      size_t JRI ; // Index of JobRotations
      DWORD BorderTime ;
      size_t LogFiles ;
      string Drive ;
      TStat(size_t ji, string drv)
       : JRI(ji), Drive(drv), BorderTime(0), LogFiles(0) {}
    };

    vector<TStat> Stats ;

    // �f�B�X�N�󂫗e�ʌv��
    for(;i<JobRotations.size();i++) {
      if(JobAborted) return ;
      string drv = lower_case(file_drive_of(JobRotations[i].FilePath)) ;
      if(spaces.find(drv)==spaces.end()) {
        __int64 space = GetDiskFreeSpaceFromFileName(drv) ;
        if(space<0) { // �󂫗e�ʕs���ׁ̈AActive �ɉ񂵂ĕ���
          LogRotateFiles(this,
            JobRotations[i].FilePath,
            JobRotations[i].Masks,
            JobRotations[i].MaxFiles,
            JobRotations[i].MaxDays,
            JobRotations[i].MaxBytes,
            JobRotations[i].SubDirectories,
            JobRotations[i].FellowMasks, JobAborted) ;
          continue ;
        }
        spaces[drv]=space ;
      }
      if( JobRotations[i].MaxFiles<INT_MAX ||
          JobRotations[i].MaxDays<INT_MAX ||
          spaces[drv]<-JobRotations[i].MaxBytes ) {
        Stats.push_back(TStat(i,drv));
      }
    }

    logset LogSet ;
    set<int> busySet ;

    // �t�@�C���̗�
    for(i=0;i<Stats.size();i++) {
      if(JobAborted) return ;
      int ji = Stats[i].JRI ;
      DWORD maxTime ;
      size_t &LogFiles = Stats[i].LogFiles ;
      size_t sz = LogSet.size() ;
      make_logset(LogSet,
        JobRotations[ji].FilePath,
        JobRotations[ji].Masks,
        JobRotations[ji].SubDirectories,&maxTime,&JobAborted,i) ;
      LogFiles = LogSet.size() - sz ;
      DWORD maxDays = DWORD(JobRotations[ji].MaxDays)<<16 ;
      if(maxTime<maxDays) Stats[i].BorderTime=0 ;
      else Stats[i].BorderTime = maxTime - maxDays ;
      busySet.insert(i);
    }

    // �t�@�C���̍폜
    for(logset::iterator pos=LogSet.begin();pos!=LogSet.end();++pos) {
      if(JobAborted) return ;
      int si = pos->tag ;
      if(!busySet.count(si)) continue ;
      int ji = Stats[si].JRI ;
      size_t &LogFiles = Stats[si].LogFiles ;
      string &LogPath = JobRotations[ji].FilePath ;
      string &drv = Stats[si].Drive ;
      if( LogFiles<=JobRotations[ji].MaxFiles &&
          pos->ftime>=Stats[si].BorderTime &&
          spaces[drv]>=-JobRotations[ji].MaxBytes ) {
        busySet.erase(si) ;
        if(busySet.empty()) break ;
      }
      string FileName = LogPath + pos->fname ;
      bool existed = file_is_existed(FileName) ;
      if(!existed||DeleteFileA(FileName.c_str())) {
        if(existed)
            LogOut("- �t�@�C�� \"%s\" ���폜���܂����B",FileName.c_str());
        __int64 bytes = existed ? pos->fsize : 0 ; LogFiles-- ;
        const masks_t &FellowMasks= JobRotations[ji].FellowMasks ;
        logset fellow_logset ;
        string fpath = file_path_of(FileName) ;
        string fpathprefix = fpath + file_prefix_of(FileName);
        make_logset(fellow_logset,fpathprefix,FellowMasks) ;
        for( logset::iterator flpos = fellow_logset.begin();
             flpos != fellow_logset.end(); flpos++ ) {
          string fellow_filename = fpath+flpos->fname ;
          if(DeleteFileA(fellow_filename.c_str())) {
            LogOut(" - �t�@�C�� \"%s\" ���폜���܂����B"
                ,fellow_filename.c_str());
            bytes += flpos->fsize ;
          }
        }
        __int64 drv_space = GetDiskFreeSpaceFromFileName(drv) ;
        if(drv_space>0) spaces[drv] = drv_space ;
        else spaces[drv] += bytes ;
      }
    }

  }
}
//---------------------------------------------------------------------------
void CMainDaemon::JobAll()
{
  JobRotate() ;
}
//---------------------------------------------------------------------------
void CMainDaemon::AddTaskIcon()
{
  if(!niEntried) {
    ZeroMemory(&niData,sizeof(NOTIFYICONDATA));
    niData.cbSize = sizeof(NOTIFYICONDATA);
    niData.hWnd   = HWMain;
    niData.uID    = 1;
    niData.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE ;
    niData.uCallbackMessage = WM_USERTASKBAR ;
    niData.hIcon  = LoadIcon(HInstance, MAKEINTRESOURCE(IDI_SMALL)) ;
    wcscpy_s(niData.szTip,sizeof(niData.szTip),_T(SPINELRD_VER));
    Shell_NotifyIcon(NIM_ADD,&niData);
    niEntried = true ;
  }
}
//---------------------------------------------------------------------------
void CMainDaemon::DeleteTaskIcon()
{
  if(niEntried) {
    Shell_NotifyIcon(NIM_DELETE,&niData);
    niEntried = false ;
  }
}
//---------------------------------------------------------------------------
bool CMainDaemon::TerminateJob(bool checkOnly)
{
  if(JobThread != INVALID_HANDLE_VALUE) {
    if(!checkOnly) JobAborted = true ;
    DWORD r = WaitForSingleObject(JobThread,checkOnly?0:THREAD_TERMINATE_LIMIT) ;
    if(r!=WAIT_OBJECT_0) {
      if(checkOnly&&r==WAIT_TIMEOUT) return false ;
      TerminateThread(JobThread,0);
    }
    CloseHandle(JobThread);
    JobThread = INVALID_HANDLE_VALUE;
  }
  return true ;
}
//---------------------------------------------------------------------------
void CMainDaemon::WMPowerBroadCast(WPARAM wParam, LPARAM lParam)
{
  switch(wParam) {
    case PBT_APMQUERYSTANDBY:
    case PBT_APMQUERYSUSPEND: //�V�X�e�����T�X�y���h��v�����Ă���
      LogOut("<�V�X�e�����T�X�y���h��v�����Ă��܂�>");
      JobTimer->SetEnabled(false) ;
      TerminateJob() ;
      break ;
    case PBT_APMSTANDBY:
    case PBT_APMSUSPEND:
      Suspended = true ;
      LogOut("<�V�X�e�����T�X�y���h�Ɉڍs���܂���>");
      ResumeTimer->SetEnabled(false) ;
      JobTimer->SetEnabled(false) ;
      TerminateJob() ;
      break ;
    case PBT_APMRESUMESUSPEND:      // ���炩�̗��R�ŃV�X�e�������A����
    case PBT_APMRESUMECRITICAL:     // �V�X�e�����v���I�ȏ�Ԃ��畜�A����
    case PBT_APMRESUMEAUTOMATIC:    // �V�X�e���������I�ɕ��A����
    case PBT_APMQUERYSUSPENDFAILED: // �V�X�e�����T�X�y���h���悤���������s����
      Suspended = false ;
      LogOut("<�V�X�e�����T�X�y���h���畜�A���܂���>");
      Resumed = true ;
      StartTimer->SetEnabled(true) ;
      break ;
  }
}

//---------------------------------------------------------------------------

  BOOL CALLBACK CloseWindowProc(HWND hWnd, LPARAM lParam)
  {
    DWORD dwProcId = (DWORD)lParam;
    DWORD dwHwndId = ~dwProcId;
    GetWindowThreadProcessId(hWnd, &dwHwndId);
    if(dwProcId==dwHwndId)
      PostMessage(hWnd, WM_CLOSE, 0, 0);
    return TRUE;
  }

void CMainDaemon::WMEndSession(WPARAM wParam, LPARAM lParam)
{
  if(wParam) {
    EndSession=true;
	LogOut("<Windows�̏I�������m���܂���>");
    if(SpinelProcess)
      EnumWindows(CloseWindowProc, (LPARAM)GetProcessId(SpinelProcess));
    Finalize();
  }
}
//---------------------------------------------------------------------------
void CMainDaemon::WMUserTaskbar(WPARAM wParam, LPARAM lParam)
{
  if(lParam==WM_LBUTTONDOWN) {
    DeleteTaskIcon() ;
    DoAboutDialog(HWMain);
    if(!QuitOrder) AddTaskIcon() ;
  }
}
//---------------------------------------------------------------------------
void CMainDaemon::OnStartTimer()
{
  // �N�����E���A��
  StartTimer->SetEnabled(false);
  LoadIni() ;
  if(first) {
    first = false ;
    LaunchSpinel() ;
    if(FullExecutingEntryOf(AppExeName())>1) {
      QuitOrder=TRUE ;
      return ;
    }
    if(DAEMONShowTaskIcon)
      AddTaskIcon();
  }
  if(!DAEMONJobPause) {
    OnJobTimer() ;
    JobTimer->SetEnabled(true) ;
  }
  ResumeTimer->SetEnabled(true) ;
}
//---------------------------------------------------------------------------
void CMainDaemon::OnResumeTimer()
{
  if(!Suspended) {
    LoadIni() ;
    if(Resumed) {
      // ���A��
      if(LaunchSpinel()) {
        if(!DAEMONJobPause) {
           OnJobTimer() ;
        }
        Resumed=false ;
      }
    }else {
      // �����Ď�
      if(SpinelDeathResume&&!EndSession) {
        if(SpinelProcess) {
          if(WaitForInputIdle(SpinelProcess,ResumeTimer->Interval())) {
            LogOut("�Ď��Ώۃv���Z�X�̎��������m���܂����B�����I�����܂��B",ResumeTimer->Interval());
            TerminateProcess(SpinelProcess,-9);
            CloseHandle(SpinelProcess);
            SpinelProcess=NULL;
          }
        }
        LaunchSpinel() ;
      }
    }
  }
}
//---------------------------------------------------------------------------
void CMainDaemon::OnJobTimer()
{
  // ���[�e�[�V����(DAEMONRotationInterval����1��)
  LoadIni() ;
  if(DAEMONJobPause) return;
  if(TerminateJob(true)) {
    TRotations rotations;
    copy(UserRotations.begin(),UserRotations.end(),back_inserter(rotations));
    JobRotations.swap(rotations);
    JobAborted = false ;
    JobThread = (HANDLE)_beginthreadex(NULL, 0, JobThreadProc, this, CREATE_SUSPENDED, NULL) ;
    if(JobThread!=INVALID_HANDLE_VALUE) {
      SetThreadPriority(JobThread,DAEMONThreadPriority);
      ResumeThread(JobThread) ;
    }
  }
}
//---------------------------------------------------------------------------
void CMainDaemon::DaemonTimerEventMain(CDaemonTimer *Timer)
{
  if(Finalized) return;
  if(Timer==StartTimer) OnStartTimer();
  else if(Timer==ResumeTimer) OnResumeTimer();
  else if(Timer==JobTimer) OnJobTimer();
}
//---------------------------------------------------------------------------
void CMainDaemon::DaemonTimerEventCallback(CDaemonTimer *Timer)
{ static_cast<CMainDaemon*>(Timer->Arg())->DaemonTimerEventMain(Timer); }
//---------------------------------------------------------------------------
unsigned int __stdcall CMainDaemon::JobThreadProc(PVOID pv)
{
  unsigned int result = 0 ;
  register CMainDaemon *this_ = static_cast<CMainDaemon*>(pv) ;
  this_->JobAll() ;
  _endthreadex(result) ;
  return result;
}
//===========================================================================
