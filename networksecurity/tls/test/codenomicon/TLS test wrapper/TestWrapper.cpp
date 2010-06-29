// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
//

#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <direct.h>


using namespace std;
STARTUPINFO startupInfo;
PROCESS_INFORMATION processInfo;
STARTUPINFO startupInfoTls;
PROCESS_INFORMATION processInfoTls;
const char* CodenomiconPath = "C:/Program Files/Codenomicon/tlsc-31/testtool/tlsc-31.jar";
const char* TestToolWrapperPath = "D:\\TlsTestWrapper\\";

int ReadConfigurations(const string& fileName, string& configSettings);
int GetPlatformAndBuild(const string& fileName, string& platform, string& build);
int RunTlsTestTool(const string& config, const string& jarPath);
int RunEpocTlsTest(const string& platform, const string& build);
int ParseLog(const string& fileDir, string& reportName);
int GetLogDirectory(const string& configFileName, string& logDir);
void ConvertPath(string& path);
int GetPlatformAndBuild(const string& fileName, string& platform, string& build);
void GetEpocDrive(string& drive);
void RemoveSpacesAround(string& inString);
int EpocPreTestConfig(const string& drive, const string& platform, const string& build);
int EpocPostTestConfig(const string& drive, const string& platform, const string& build);
void WaitForEpocTest();

/* this function is used to read the configuration settings from the config file
   for Codenomicon TLS Client test tool

   its parameters are:
   1) fileName(in): this contains the name of the config file
   2) configSettings(out): on return, this contains the configuration settings
*/
int ReadConfigurations(const string& fileName, string& configSettings)
    {
    ifstream configFile(fileName.c_str());
    if(configFile.is_open())
        { 
        string configLine;

        // go to the codenomicon params section
        getline(configFile,configLine);
        while(configLine.find("[params]", 0) == string::npos)
            {
            getline(configFile,configLine);
            if(configFile.eof())
                {
                cout<<"\ncould not find [params] section in config file"<<endl;
                return -1;
                }
            }

        // reached the params section, now read the configuration settings
        while(!configFile.eof())
            {
            getline(configFile, configLine);

            if(!configLine.empty())
                {
                // chop off any blank spaces before the config options
                while(configLine[0] == ' ')
                    {
                    configLine.erase(0, 1);
                    }

                // add to config string only if the line is not a comment
                if(configLine[0] != '/' && configLine[1] != '/')
                    {
                    // codenomicon expects each config setting to be preceeded with --
                    configSettings = configSettings + " --" + configLine;
                    }
                }
            }

        configFile.close();
        }
    else
        {
        cout << "Unable to open config file: "<< fileName;
        return -1;
        }
    }

/* this function is used to remove spaces before as well as after a word

   its parameter is:
   1) inString (in/out): this contains the input string and on return is contains
      the string without any spaces before or after the word
*/

void RemoveSpacesAround(string& inString)
    {
    int i = 0;
    while(inString[i] == ' ')
        {
        inString.erase(i, 1);
        ++i;
        }

    i = inString.length();
    while(inString[i] == ' ')
        {
        inString.erase(i, 1);
        --i;
        }

    }

/* this function is used to read the platform and build from the config file for
   which the test has to be run

   its parameters are:
   1) fileName(in): this contains the name of the config file
   2) platform(out): on return, this contains the platform
   3) build(out): on return, this contains the build
*/

int GetPlatformAndBuild(const string& fileName, string& platform, string& build)
    {
    ifstream configFile(fileName.c_str());
    if(!configFile.is_open())
        {
        cout<<"Failed to open config file: "<<fileName<<endl;
        return -1;
        }

    string configLine;
    bool foundPlatform = false;
    bool foundBuild = false;
    int posPlatform = -1;
    int posBuild = -1;


    // read the platform and build section from the file in a single pass
    while(!configFile.eof())
        {
        getline(configFile,configLine);
        posPlatform = configLine.find("[platform]", 0);
        if(posPlatform != string::npos)
            {
            // found platform section, its value is after the [platform], i.e. 10 positions after it
            platform = configLine.substr(posPlatform + 10, configLine.length());
            RemoveSpacesAround(platform);
            foundPlatform = true;
            if(foundBuild)
                {
                configFile.close();
                return 0;
                }
            }

        posBuild = configLine.find("[build]", 0);
        if(posBuild != string::npos)
            {
            // found the build section, its value is after the [build], i.e. 7 positions after it
            build = configLine.substr(posBuild + 7, configLine.length());
            RemoveSpacesAround(build);
            foundBuild = true;
            if(foundPlatform)
                {
                configFile.close();
                return 0;
                }
            }
        }

        // could not find either platform or build or both
        if(!foundPlatform)
            {
            cout<<"\nCould not find Platform section"<<endl;
            }
        if(!foundBuild)
            {
            cout<<"Could not find Build section"<<endl;
            }

        configFile.close();
        return -1;
    }


/* this function runs the TLS test tool i.e. Codenomicon.

   its parameters are:
   1) config(in): the configuration options for the Codenomicon tool
   2) jarPath(in): the path of the Codenomicon jar file
*/

int RunTlsTestTool(const string& config, const string& jarPath)
    {
    string command = "java -Xmx128M -jar " + jarPath + config;
    //cout<<endl<<command<<endl;

    //return system(command.c_str());


    ZeroMemory(&startupInfoTls, sizeof(startupInfoTls));
    startupInfoTls.cb = sizeof(startupInfoTls);

    ZeroMemory(&processInfoTls, sizeof(processInfoTls));

  //  string tlsTestProgram = driveLetter +":/epoc32/RELEASE/"+ platform +"/"+ build +"/TlsClientTest";
    //cout<<"\ntls test program path: "<<tlsTestProgram<<endl;

    cout<<"Launching Codenomicon TLS test tool"<<endl;
    // create a separate process for Codenomicon TLS test tool
    if(!CreateProcess( NULL,   // no module name (use command line)
        (char*)command.c_str(),  // Codenomicon TLS test tool
        NULL,             // process handle not inheritable
        NULL,             // thread handle not inheritable
        FALSE,            // set handle inheritance to FALSE
        0,                // no creation flags
        NULL,             // use parent's environment block
        NULL,             // use parent's starting directory
        &startupInfoTls,     // pointer to STARTUPINFO structure
        &processInfoTls)     // pointer to PROCESS_INFORMATION structure
      )
        {
        int ret = GetLastError();
        cout<<"CreateProcess for Codenomicon TLS test tool failed: "<<ret<<endl;
        return ret;
        }

    return 0;
    }

/* this function parses the log file and prepares a report file

   its parameters are:
   1) logDir (in): the logging directory
   2) reportName (out): name of the report file
*/

int ParseLog(const string& logDir, string& reportName)
    {
    int totalError = 0; // keeps track of the total number of errors
    int length = logDir.length();

    string summaryFileName; // stores the name of the summary file generated by Codenomicon
    if(logDir[length-1] == '/')
        {
        summaryFileName = logDir + "summary.txt";
        }
    else
        {
        summaryFileName = logDir + "/summary.txt";
        }

    string logFileName; // stores the name of log file generated by Codenomicon
    if(logDir[length-1] == '/')
        {
        logFileName = logDir + "main.log";
        }
    else
        {
        logFileName = logDir + "/main.log";
        }

    // create a report file in the log directory
    string report = logDir + reportName;
    ofstream outFile(report.c_str());
    if(!outFile.is_open())
        {
        cout << "Unable to create report file";
        return -1;
        }

    // open the summary file and copy its contents to the report file
    // also find out the total number of tests run
    int totalTests = 0;

    ifstream summaryFile(summaryFileName.c_str());
    if(summaryFile.is_open())
        {
        string summaryLine;
        while(!summaryFile.eof())
            {
            getline(summaryFile, summaryLine);

            // check if the line has total number of tests
            if(summaryLine.find("Test cases", 0) != string::npos)
                {
                // found the line, now get the number from the string
                // the number is in the end of the line and it has spaces before it
                int len = summaryLine.length();
                int i = len;
                while(summaryLine[i--] != ' ')
                    { }

                string totalTestsString = summaryLine.substr(i, summaryLine.length());
                totalTests = atoi(totalTestsString.c_str());
                }
            }

        summaryFile.close();
        }
    else
        {
        cout << "Unable to open summary file generated by Codenomicon";
        outFile.close();
        return -1;
        }

    // open the log file
    ifstream logFile(logFileName.c_str());
    string line;
    string nextLine;

    if(!logFile.is_open())
        {
        cout << "Unable to open log file";
        outFile.close();
        return -1;
        }

    //outFile<<"List of test cases failed"<<endl<<"========================="<<endl;
    while(!logFile.eof())
        {
        getline (logFile,line);
        if((!line.empty()) && ((line.find("ERROR", 0) != string::npos) || (line.find("error", 0) != string::npos)))
            {
     /*       if(line.find("ERROR Expected", 0) == string::npos)       */
                {
                // read next line and check if it contains the string "Test case #"
                getline(logFile,nextLine);
                if((!nextLine.empty()) && (nextLine.find("Test case #", 0) != string::npos))
                    {
                    totalError++;

                    // write line and nextLine to report
                    //outFile<<line<<endl<<nextLine<<endl;
                    }
                else
                    {
                    // get the next line as sometimes a line is in between for events and octets
                    getline(logFile, nextLine);
                    if((!nextLine.empty()) && (nextLine.find("Test case #", 0) != string::npos))
                        {
                        totalError++;

                        // write line and nextLine to report
                        //outFile<<line<<endl<<nextLine<<endl;
                        }
                    }
                }
            }
        }

    outFile<<totalTests-totalError<<endl; // print total no of tests passed
    outFile<<totalError<<endl;            // print total no of tests failed
    logFile.close();
    outFile.close();

    return 0;
    }

/* this function gets the log directory by reading the config file

   its parameters are:
   1) configFileName (in): the name of the config file
   2) logDir (out): on return contains the logging directory
*/

int GetLogDirectory(const string& configFileName, string& logDir)
    {
    ifstream configFile(configFileName.c_str());
    string configLine;

    if(configFile.is_open())
        {
        while(!configFile.eof())
            {
            getline(configFile,configLine);
            if(!configLine.empty())
                {
                unsigned int i = configLine.find("log-dir", 0);
                if(i != string::npos)
                     {
                     // found line containing log-dir, extract log directory
                     logDir = configLine;

                     // erase from first char of the line to the end of "log-dir"
                     logDir.erase(0, i+7);

                     // chop off any blank spaces before log directory name
                     while(logDir[0] == ' ')
                         {
                         logDir.erase(0, 1);
                         }
                     }
                }
            }

        configFile.close();
        }
        else
        {
        cout << "Unable to open file";
        return -1;
        }

    }

/* this function converts backward slashes "\" to forward slashes "/" .
   this is needed to prevent interpretation of backward slashes as format
   specifiers.

   its parameter is:
   1) path (in/out): on calling, it contains the path possibly containing backward
      slashes. on returning, it has path containing no backward slashes
*/

void ConvertPath(string& path)
    {
    for(int i = 0; i<path.length(); i++)
        {
        if(path[i] == '\\')
            {
            path[i] = '/';
            }
        }
    }

/* this function gets the epoc drive

    its parameters are:
    1) driveLetter (out): on return contains the epoc drive
*/

void GetEpocDrive(string& driveLetter)
    {
    // get the current drive
    char currentDrive = _getdrive();
    currentDrive = currentDrive + 'A' -1;

    driveLetter = "C";
    driveLetter[0] = currentDrive;
    }

/*  this function launches the epoc side TLS test.

    its parameters are:
    1) drive (in): the epoc drive
    2) platform (in): the platform for which the test will be run (e.g. WINSCW)
    3) build (in): the build for which the test will be run (e.g. UDEB)
*/

int RunEpocTlsTest(const string& driveLetter, const string& platform, const string& build)
    {
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    ZeroMemory(&processInfo, sizeof(processInfo));

    string tlsTestProgram = driveLetter +":/epoc32/RELEASE/"+ platform +"/"+ build +"/TlsClientTest";


    //before we launch the TLS test program we have to configure Epoc
    cout<<"\nConfiguring Epoc for TLS test program"<<endl;
    int ret = EpocPreTestConfig(driveLetter, platform, build);
    if(ret != 0)
        {
        return ret;
        }

    cout<<"Launching epoc client side test"<<endl;
    // create a separate process for epoc test
    if(!CreateProcess( NULL,   // no module name (use command line)
        (char*)tlsTestProgram.c_str(),  // epoc TLS client test
        NULL,             // process handle not inheritable
        NULL,             // thread handle not inheritable
        FALSE,            // set handle inheritance to FALSE
        0,                // no creation flags
        NULL,             // use parent's environment block
        NULL,             // use parent's starting directory
        &startupInfo,     // pointer to STARTUPINFO structure
        &processInfo)     // pointer to PROCESS_INFORMATION structure
      )
        {
        int ret = GetLastError();
        cout<<"CreateProcess failed: "<<ret<<endl;
        cout<<"Tried to execute: "<<tlsTestProgram<<endl;

        // restore the original settings for epoc
        ret = EpocPostTestConfig(driveLetter, platform, build);
        if(ret != 0)
            {
            cout<<"Failed to restore original commsDat settings: "<<ret<<endl;
            }
        return ret;
        }

    return 0;
    }

/*  this function configures Epoc for running the TLS test. this involves backing
    up the current commsDat and configuring the commsDat for loopback using WinTAP

    its parameters are:
    1) drive (in): the drive on which epoc is mapped
    2) platform (in): the platform (e.g. WINSCW)
    3) build (in): the build (e.g. UDEB)
*/

int EpocPreTestConfig(const string& drive, const string& platform, const string& build)
    {
    string testToolPath = TestToolWrapperPath;

    // create a directory for backup
    string backupDir = "mkdir "+ testToolPath + "backup";
    int ret = system(backupDir.c_str());

    // make backup of existing epoc.ini and secdlg.dll
    string backupEpoc = "copy /Y " + drive + ":\\epoc32\\data\\epoc.ini " + testToolPath + "backup\\epoc.ini";
    ret = system(backupEpoc.c_str());
  /*  if(ret != 0)
        {
        cout<<"failed to copy epoc.ini: "<<endl<<backupEpoc<<endl<<" failed"<<endl;
        return ret;
        }
    */
    string backupSec = "copy /Y " + drive + ":\\epoc32\\release\\" + platform + "\\" + build + "\\secdlg.dll " + testToolPath + "backup\\secdlg.dll";
    ret = system(backupSec.c_str());
  /*  if(ret != 0)
        {
        cout<<"failed to copy secdlg.dll: "<<endl<<backupSec<<endl<<" failed"<<endl;
        return ret;
        } */

    // delete secdlg.dll from epoc drive
    string deletesecdlg = "del " + drive + ":\\epoc32\\release\\" + platform + "\\" + build + "\\secdlg.dll ";
    ret = system(deletesecdlg.c_str());
 /*   if(ret != 0)
        {
        cout<<"failed to delete secdlg.dll: "<<endl<<deletesecdlg<<endl<<" failed"<<endl;
        return ret;
        }
   */
    // copy the epoc_shell.ini, wintapstaticnogateway.xml, ethertap.pdd and tsecdlg.dll to epoc

    string copyShellIni = "copy /Y " + testToolPath + "epoc_shell.ini " + drive + ":\\epoc32\\data\\epoc.ini";
    ret = system(copyShellIni.c_str());
    if(ret != 0)
        {
        cout<<"failed to copy epoc.ini: "<<endl<<copyShellIni<<endl<<" failed"<<endl;
        return ret;
        }

    string copyXml = "copy /Y " + testToolPath + "wintapstaticnogateway.xml " + drive + ":\\epoc32\\" + platform + "\\c";
    ret = system(copyXml.c_str());
    if(ret != 0)
        {
        cout<<"failed to copy wintapstaticnogateway.xml: "<<endl<<copyXml<<endl<<" failed"<<endl;
        return ret;
        }

    string copyPdd = "copy /Y " + testToolPath + "ethertap.pdd " + drive + ":\\epoc32\\release\\" + platform + "\\udeb";
    ret = system(copyPdd.c_str());
    if(ret != 0)
        {
        cout<<"failed to copy ethertap.pdd: "<<endl<<copyPdd<<endl<<" failed"<<endl;
        return ret;
        }

    string copyDll = "copy /Y " + testToolPath + "tsecdlg.dll " + drive + ":\\epoc32\\release\\" + platform + "\\udeb";
    ret = system(copyDll.c_str());
    if(ret != 0)
        {
        cout<<"failed to copy tsecdlg.dll: "<<endl<<copyDll<<endl<<" failed"<<endl;
        return ret;
        }

    // launch 'ceddump' to retrieve the current commsDat configuration
    string cedDump = drive + ":/epoc32/release/" + platform + "/" + build + "/ceddump";
    ret = system(cedDump.c_str());
    if(ret != 0)
        {
        cout<<"failed to launch ceddump: "<<endl<<cedDump<<endl<<"failed"<<endl;
        return ret;
        }

    // delete the old backup file, if it exists
    string delBackup = "del " + drive + ":\\epoc32\\" + platform + "\\c\\tlsbackup.cfg";
    cout<<"\ntrying to delete old backup file: tlsbackup.cfg"<<endl;
    ret = system(delBackup.c_str());
    if(ret != 0)
        {}

    // make a backup of current commsDat configuration
    string backup = "rename " + drive + ":\\epoc32\\" + platform + "\\c\\cedout.cfg tlsbackup.cfg";
    ret = system(backup.c_str());
    if(ret != 0)
        {
        cout<<"failed to save current commsDat: "<<endl<<backup<<endl<<"failed"<<endl;
        return ret;
        }

    // set up commsDat for loopback using WinTAP
    string winTap = drive + ":\\epoc32\\release\\" + platform + "\\" + build + "\\ced c:\\WinTapStaticNoGateway.xml";
    ret = system(winTap.c_str());
    if(ret != 0)
        {
        cout<<"failed commsDat setup for WinTap: "<<endl<<winTap<<endl<<"failed"<<endl;
        return ret;
        }
    // copy epoc.ini so that the techview boots up in GUI mode - the test runs only in GUI mode
    string copyIni = "copy /Y " + testToolPath + "epoc.ini " + drive + ":\\epoc32\\data\\epoc.ini";
    ret = system(copyIni.c_str());
    if(ret != 0)
        {
        cout<<"failed to copy epoc.ini: "<<endl<<copyIni<<endl<<" failed"<<endl;
        return ret;
        }


    return 0;
    }

/* this function restores the commsDat setup to the original settings (i.e. to that
   of before running TLS test

    its parameters are:
    1) drive (in): the drive on which epoc is mapped
    2) platform (in): the platform (e.g. WINSCW)
    3) build (in): the build (e.g. UDEB)
*/

int EpocPostTestConfig(const string& drive, const string& platform, const string& build)
    {
    string testToolPath = TestToolWrapperPath;

    string copyShellIni = "copy /Y " + testToolPath + "epoc_shell.ini " + drive + ":\\epoc32\\data\\epoc.ini";
    int ret = system(copyShellIni.c_str());
    if(ret != 0)
        {
        cout<<"failed to copy epoc.ini: "<<endl<<copyShellIni<<endl<<" failed"<<endl;
        return ret;
        }

    // restore previous commsDat setup
    string prevSetting = drive + ":\\epoc32\\release\\" + platform + "\\" + build + "\\ced c:\\tlsbackup.cfg";
    ret = system(prevSetting.c_str());
    if(ret != 0)
        {
        cout<<"failed to restore previous commsDat setup: "<<endl<<prevSetting<<endl<<"failed"<<endl;
        return ret;
        }

    // rename the backup file to original
    string backup = "rename " + drive + ":\\epoc32\\" + platform + "\\c\\tlsbackup.cfg cedout.cfg";
    ret = system(backup.c_str());
    if(ret != 0)
        {
        cout<<"failed to restore backup file: "<<endl<<backup<<endl<<"failed"<<endl;
        return ret;
        }

    // restore original epoc.ini and secdlg.dll
    string backupEpoc = "copy /Y " + testToolPath + "backup\\epoc.ini " + drive + ":\\epoc32\\data\\epoc.ini " ;
    ret = system(backupEpoc.c_str());
    if(ret != 0)
        {
        cout<<"failed to copy epoc.ini: "<<endl<<backupEpoc<<endl<<" failed"<<endl;
        return ret;
        }

    string backupSec = "copy /Y " + testToolPath + "backup\\secdlg.dll " + drive + ":\\epoc32\\release\\" + platform + "\\" + build + "\\secdlg.dll " ;
    ret = system(backupSec.c_str());
    if(ret != 0)
        {
        cout<<"failed to copy secdlg.dll: "<<endl<<backupSec<<endl<<" failed"<<endl;
        return ret;
        }
    // delete tsecdlg.dll from epoc drive
    string deletesecdlg = "del " + drive + ":\\epoc32\\release\\" + platform + "\\" + build + "\\tsecdlg.dll ";
    ret = system(deletesecdlg.c_str());
    if(ret != 0)
        {
        cout<<"failed to delete tsecdlg.dll: "<<endl<<deletesecdlg<<endl<<" failed"<<endl;
        return ret;
        }


    return 0;

    }

/*  this function waits for Epoc TLS client test to finish
*/

void WaitForEpocTest()
    {
    DWORD dwExitCode;
    GetExitCodeProcess(processInfo.hProcess, &dwExitCode);
    while(dwExitCode == STILL_ACTIVE)
        {
        cout<<".";
        GetExitCodeProcess(processInfo.hProcess, &dwExitCode);
        Sleep(1000);
        }
    }

bool ProcessRunning(HANDLE handle)
    {
    DWORD dwExitCode;
    GetExitCodeProcess(handle, &dwExitCode);
    if(dwExitCode == STILL_ACTIVE)
        {
        return true;
        }
    else
        {
        return false;
        }
    }

int PingEpoc()
    {
    int ret = system("ping -n 1 -w 20000  192.168.0.2");
    return ret;
    }

/* this program takes one argument:
   1) config file in text format
*/
int main(int argc, char* argv[])
    {
    if(argc != 2)
        {
        cout << argv[0]
             << "  [config file] "<<endl;

        return -1;
        }

    string fileName = argv[1];

    string configSettings = " --no-gui ";
    int ret = ReadConfigurations(fileName, configSettings);
    if(ret != 0)
        {
        return ret;
        }

    string drive;
    string platform;
    string build;

    GetEpocDrive(drive);

    ret = GetPlatformAndBuild(fileName, platform, build);
    if(ret != 0)
        {
        return ret;
        }

    // kill the previously running tlsclienttest program, if any
 //   cout<<"zzz before killing"<<endl;
 //   ret = system("TASKKILL /F /IM tlsclienttest.exe /T");
  //  cout<<"zzz after killing1: "<<ret<<endl;
 //   ret = system("TASKKILL /F /IM java.exe /T");
  //  cout<<"zzz after killing2: "<<ret<<endl;
    // run epoc side TLS test client
    ret = RunEpocTlsTest(drive, platform, build);
    if(ret != 0)
        {
        cout<<"Exiting program"<<endl;
        return ret;
        }

    string jarPath = CodenomiconPath;
    jarPath += "\"";
    string quotedJarPath = "\"" + jarPath;

    // run Codenomicon TLS Client test tool
    cout<<"Launching Codenomicon TLS Client test tool"<<endl;
    ret = RunTlsTestTool(configSettings, quotedJarPath);
    if(ret != 0)
        {
        return ret;
        }

    // now Epoc TLS Client test should be executed till Codenomicon tool is running
/*    while(TlsTestToolRunning())
        {
        WaitForEpocTest();

        // check if the Codenomicon test tool is still running
        if(TlsTestToolRunning())
            {
            // Codenomicon test tool still running, run epoc side TLS test client again
            ret = RunEpocTlsTest(drive, platform, build);
            if(ret != 0)
                {
                cout<<"Exiting program"<<endl;
                return ret;
                }
            }
        }
  */

    HANDLE handles[2];
    handles[0] = processInfo.hProcess;
    handles[1] = processInfoTls.hProcess;
 /* Commented code starts-*/
    // wait for any of the process to finish
    WaitForMultipleObjects(2, handles, false, INFINITE);

    // if epoc side TLS test finished and Codenomicon
    // tool is still running, then terminate Codenomicon process
    if(!ProcessRunning(processInfo.hProcess))
        {
        if(ProcessRunning(processInfoTls.hProcess))
            {
            // not a clean way, but have to terminate Codenomicon test tool
            TerminateProcess(processInfoTls.hProcess, 1);
            }
        }

    // if Codenomicon test tool is finished, then terminate the epoc side test
    if(!ProcessRunning(processInfoTls.hProcess))
        {
        if(ProcessRunning(processInfo.hProcess))
            {
            // not a clean way, but we have to terminate epoc test
            TerminateProcess(processInfo.hProcess, 1);
            }
        }

 /*   commented code ends*/

    // change started ****************

   // while(ProcessRunning(processInfo.hProcess) &&   ProcessRunning(processInfoTls.hProcess))
 /*  while(1)
        {
        // wait for any of the process to finish
        WaitForMultipleObjects(2, handles, false, 20000);

        // if epoc side TLS test finished and Codenomicon
        // tool is still running, then terminate Codenomicon process
        if(!ProcessRunning(processInfo.hProcess))
            {
            cout<<"\nepoc finished, terminating codenomicon"<<endl;
            if(ProcessRunning(processInfoTls.hProcess))
                {
                // not a clean way, but have to terminate Codenomicon test tool
                TerminateProcess(processInfoTls.hProcess, 1);
                return 0;
                }
            }

        // if Codenomicon test tool is finished, then terminate the epoc side test
        if(!ProcessRunning(processInfoTls.hProcess))
            {
            cout<<"\ncodenomicon finished, terminating epoc"<<endl;
            if(ProcessRunning(processInfo.hProcess))
                {
                // not a clean way, but we have to terminate epoc test
                TerminateProcess(processInfo.hProcess, 1);
                return 0;
                }
            }

        // check if the epoc side TLS test has crashed
        ret = PingEpoc();
        if(ret != 0)
            {
            // epoc not responding, kill the processes and return
            if(ProcessRunning(processInfoTls.hProcess))
                {
                TerminateProcess(processInfoTls.hProcess, 1);
                }

            if(ProcessRunning(processInfo.hProcess))
                {
                TerminateProcess(processInfo.hProcess, 1);
                }

            return ret;
            }
        }

      */

// change ends ****************

    /* now that the test tool has executed, we need to get the results from the log file
       but first we need the log directory for reading the log file
    */
  /*  string logDir;
    ret = GetLogDirectory(fileName, logDir);
    if(ret != 0)
        {
        cout<<"\nwaiting for Epoc TLS Client test to exit";
        // wait for epoc tls test to finish
        WaitForEpocTest();

        // restore previous commsDat settings
        ret = EpocPostTestConfig(drive, platform, build);
        return ret;
        }
    */
    // convert the backward slashes to forward slashes in the path
 //   ConvertPath(logDir);

	// prepare a report from the log file
  //  string outFile("report.txt");
  //  ret = ParseLog(logDir, outFile);

    // wait for epoc tls test to finish
 //   cout<<"\nwaiting for Epoc TLS Client test to exit";
 //   WaitForEpocTest();

    // restore previous commsDat settings
//    cout<<"\nrestoring previous commsDat settings"<<endl;
    ret = EpocPostTestConfig(drive, platform, build);

    return ret;
    }

