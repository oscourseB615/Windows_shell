#define _WIN32_WINNT 0x0501         /*Windows API版本*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winbase.h>
#include <Wincon.h>
#include <tlhelp32.h>
#include <malloc.h>
#include <string.h>
#include <direct.h>
#include "WinShell.h"

HANDLE for_bg;            //后台进程
HANDLE for_fp;            //前台进程 
HANDLE wait_sig[2];
          
int main()
{
	/******************************声明程序中用到的函数**************************************/

	void cd_cmd(char *dir);                     /*显示cd命令*/
	void dir_cmd(char *dir,char kind);                    /*显示dir命令*/
	void ftime(FILETIME filetime);              /*显示文件创建时间*/
	void GetProcessList();                      /*获得系统当前进程列表*/
	void history_cmd();                         /*获得最近输入的命令*/
	void add_history(char *inputcmd);           /*将输入的命令添加到命令历史中*/
	HANDLE process(int bg, char appName[]);                     /*创建进程*/
    BOOL killProcess(char *pid);                /*kill进程*/
    BOOL WINAPI ConsoleHandler(DWORD CEvent);   /*回调函数*/
	void help();                                /*显示帮助信息*/
	void transfer(unsigned long size);          /*dir/c转换size*/
	void pipe(char appName1[],char appName2[]); /*pipe*/
	void susp(int id);
	void resp(int id);

	char c, *input, *arg[3], path[BUFSIZE];
	int input_len = 0, is_bg = 0, i, j, k;
	HANDLE hprocess;              /*进程执行结束，返回进程句柄*/
	DWORD dwRet;
    wait_sig[1] = CreateEvent(NULL,TRUE,FALSE,NULL);
    /*设置钩子，捕捉ctrl+c命令，收到即结束进程*/		
		if(SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler,TRUE) == FALSE)
		{
			printf("Unable to install handler!\n");
				return NULL;
		} 
	while(1)
	{    
		/*将指向输入命令的指针数组初始化*/
		for(i= 0; i < 3; i++)
			arg[i] = NULL;
		/*获得当前目录，返回的地址存入“path”中，BUFSIZE是最多能够保存的地址长度*/
		dwRet = GetCurrentDirectory(BUFSIZE, path);
		
		if( dwRet == 0 )
		{   
			/*返回当前目录失败，输出出错信息*/
			printf("GetCurrentDirectory failed (%d)\n", GetLastError());
			
		}
		else if(dwRet > BUFSIZE)
		{
			/*BUFSIZE长度小于返回地址长度，输出还需多少长度*/
			printf("GetCurrentDirectory failed (buffer too small; need %d chars)\n", dwRet);
			
		}
		else
			/*输出当前目录*/
			printf("%s>", path);

		/************************************输入***********************************************/
		
		input_len = 0;
		/*将无用字符过滤掉*/
		while((c = getchar()) ==  ' ' || c == '\t' || c == EOF)
		{
		}
		if(c == '\n')                   /*输入为空时结束本次循环打印提示符*/
			continue;
		while(c != '\n')
		{
			buf[input_len++] = c;
			c = getchar();
			
		}
		buf[input_len++] =  '\0';       /*加上串结束符*/
		
		/*分配动态存储空间，将命令从缓存复制到input中*/
		input = (char*) malloc(sizeof(char)*(input_len));
		strcpy(input, buf);
 
		/************************************解析指令********************************************/
		
		// TODO: 对输入命令进行解析，并存储到字符串数组中
		for(i=0,j=0,k=0;i<input_len;i++)
		{
			if(input[i]==' '||input[i]=='\0')
			{
				if(j==0)
					continue;
				else
				{
					buf[j++]='\0';
					arg[k]=(char*)malloc(sizeof(char)*j);
					strcpy(arg[k++],buf);
					j=0;
				}
			}
			else
			{
			buf[j++]=input[i];
			}
			
		}


		/*********************************内部命令处理******************************************/
      
		// TODO: 根据解析结果，进入相应的命令处理函数
		/*cd 命令*/
		if(strcmp(arg[0],"cd")==0)
		{
			add_history(input);
            cd_cmd(arg[1]);
			free(input);
			continue;
		}

		/*dir 命令*/
		if(strcmp(arg[0],"dir")==0||strcmp(arg[0],"dir/A")==0||strcmp(arg[0],"dir/B")==0||strcmp(arg[0],"dir/C")==0)
		{
			char kind='A';
            char *route;
			add_history(input);
			if(strcmp(arg[0],"dir/B")==0)
				kind='B';
			else if(strcmp(arg[0],"dir/C")==0)
				kind='C';
			if(arg[1]==NULL) /*没有输入dir目录参数，dir目录为当前目录*/
			{
				route=path;
				dir_cmd(route,kind);
			}
			else
				dir_cmd(arg[1],kind);
			free(input);
			continue;
		}

		/*tasklist 命令*/
		if(strcmp(arg[0],"tasklist")==0)
		{
			add_history(input);
			GetProcessList();
			free(input);
			continue;
		}

		/*前台进程*/
        if(strcmp(arg[0],"fp")==0)
		{
			add_history(input);
			if(arg[1]==NULL)
			{
				printf("没有指定可执行文件\n");
				free(input);
				continue;
			}
			is_bg=0;
			hprocess=process(is_bg,arg[1]);
			for_fp=hprocess;
			wait_sig[0]=hprocess;
			//if(WaitForSingleObject(hprocess,INFINITE)==WAIT_OBJECT_0)
				if(WaitForMultipleObjects(2,wait_sig,FALSE,-1)==WAIT_OBJECT_0)
				free(input);
			continue; 
		}

		/*后台进程*/
		if(strcmp(arg[0],"bg")==0)
		{
			add_history(input);
			if(arg[1]==NULL)
			{
				printf("没有指定可执行文件\n");
				free(input);
				continue;
			}
			is_bg=1;
			hprocess=process(is_bg,arg[1]);
			for_bg=hprocess;
		
			free(input);
			continue;
		}

		/*kill进程*/
		if(strcmp(arg[0],"taskkill")==0)
		{
			BOOL success;
			add_history(input);
			success=killProcess(arg[1]);
			if(!success)
				printf("kill process failed!\n");
			free(input);
			continue;
		}

		/*显示历史命令*/
		if(strcmp(arg[0],"history")==0)
		{
			add_history(input);
			history_cmd();
			free(input);
			continue;
		}

		/*exit 命令*/
		if(strcmp(arg[0],"exit")==0)
		{
			add_history(input);
			printf("Bye bye!\n");
			free(input);
			break;
		}
	    
		/*help 命令*/
		if(strcmp(arg[0],"help")==0)
		{
			add_history(input);
			help();
			free(input);
			continue;
		}
		
        /*后台转前台*/
		if(strcmp(arg[0],"tofp")==0)
		{
			add_history(input);
			if(WaitForSingleObject(for_bg,INFINITE)==WAIT_OBJECT_0)
            free(input);
			continue;
		}
		/*pipe*/
		if(arg[1]!=NULL&&strcmp(arg[1],"|")==0)
		{
			add_history(input);
			if(arg[2]!=NULL)
			pipe(arg[0],arg[2]);
			free(input);
			continue;
		}
		else
		{
			printf("please type in correct command!\n");
			continue;
		}
	}	
}

/**********************************主程序完*******************************************/


/*********************************相关命令处理函数*****************************************/


/**********************************cd命令**********************************************/

void cd_cmd(char *route)
{
	
	if( !SetCurrentDirectory(route))                /*设置当前目录，失败返回出错信息*/   
	{
		printf(TEXT("SetCurrentDirectory failed (%d)\n"), GetLastError());	
	}
}


/************************************dir命令*********************************************/

void dir_cmd(char *route,char kind)
{
	
	WIN32_FIND_DATA FindFileData;                /*将查找到的文件或目录以WIN32_FIND_DATA结构返回*/
	files_Content head, *p, *q;	                 /*定义指向文件结构体的指针*/
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError;
	char volume_name[256];
	int file = 0, dir = 0;                             /*文件数目和目录数目初始值为0*/
	__int64 sum_file = 0;                            /*总文件大小用Bytes显示值较大，保存为64位整数*/
	__int64 l_user, l_sum, l_idle;                 /*已用空间，总容量，可用空间*/
	unsigned long volume_number;
	char *DirSpec[3];
	
	DirSpec[0] = (char*)malloc(sizeof(char) * 2);	
	strncpy(DirSpec[0], route, 1);	
	*(DirSpec[0] + 1) = '\0';                      /*DirSpec[0]为驱动器名*/
	DirSpec[1] = (char*)malloc(sizeof(char) * 4);
    strcpy(DirSpec[1], DirSpec[0]); 
	strncat(DirSpec[1], ":\\", 3);                  /*DirSpec[1]用于获得驱动器信息*/
    DirSpec[2] = (char*)malloc(sizeof(char) * (strlen(route) + 2));
    DirSpec[3] = (char*)malloc(sizeof(char) * (strlen(route) + 5));
	strcpy(DirSpec[2], route);
	strcpy(DirSpec[3], route);
	strncat(DirSpec[2], "\\", 2);	              /*DirSpec[2]为dir命令的目录名*/
	strncat (DirSpec[3], "\\*.*", 5);             /*DirSpec[3]用于查找目录中的文件*/
	
	hFind = FindFirstFile(DirSpec[3], &FindFileData);	
	if (hFind == INVALID_HANDLE_VALUE)            /*查找句柄返回为无效值，查找失败*/
	{
		printf ("Invalid file handle. Error is %u\n", GetLastError());
		
	} 
	else 
	{   
		/*获取卷的相关信息*/
		GetVolumeInformation(DirSpec[1], volume_name, 50, &volume_number, NULL, NULL, NULL, 10);
		
		if(strlen(volume_name) == 0)
			printf(" 驱动器 %s 中的卷没有标签。\n", DirSpec[0]);
		else
			printf(" 驱动器 %s 中的卷是 %s \n", DirSpec[0], volume_name); 		
		printf(" 卷的序列号是 %X \n\n", volume_number);   
		printf("  %s 的目录 \n\n", DirSpec[2]);		
		head.time = FindFileData.ftCreationTime;              /*获得文件创建时间，存入文件结构体*/
        strcpy(head.name, FindFileData.cFileName);             /*获得文件名，存入文件结构体*/
		/*如果数据属性是目录，type位为0*/
		if( FindFileData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)					
		{
			head.type = 0;
			dir++;
		}
		else			
		{
			/*如果数据属性是文件，type位为1*/
			head.type = 1;
			head.size = FindFileData.nFileSizeLow;           /*将文件大小存入结构体中*/
			file++;
			sum_file += FindFileData.nFileSizeLow;             /*将文件大小累加*/
		}		
        p = &head;
		/*如果还有下一个数据，继续查找*/
		while (FindNextFile(hFind, &FindFileData) != 0) 
		{
			
			q = (files_Content*)malloc(sizeof(files_Content));
			q->time = FindFileData.ftCreationTime;
			strcpy(q->name, FindFileData.cFileName);
			if( FindFileData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
			{
				q->type = 0;
				dir++;
			}
			else
			{
				q->type = 1;
				q->size = FindFileData.nFileSizeLow;
				file++;
				sum_file += FindFileData.nFileSizeLow;
			}			
			p->next = q;
			p = q;
			
		}
		p->next = NULL;
		p = &head;
		/*将结构体中数据的创建时间、类型、大小、名称等信息依次输出*/
		while(p != NULL)
		{
			if(kind!='B')
			{
			    ftime(p->time);
			    if(p->type == 0)
				    printf("\t<DIR>\t\t");
		     	else
				{
					if(kind!='C')
			     	printf("\t\t%9lu", p->size);
					else
						transfer(p->size);
				}
			}
			printf ("\t%s\n", p->name);		
			p = p->next;
		}
		free(p);
		/*显示文件和目录总数，磁盘空间相关信息*/
		printf("%15d 个文件\t\t\t%I64d 字节 \n", file, sum_file);
		GetDiskFreeSpaceEx(DirSpec[1], (PULARGE_INTEGER)&l_user, (PULARGE_INTEGER)&l_sum, (PULARGE_INTEGER)&l_idle);	
		printf("%15d 个目录\t\t\t%I64d 可用字节 \n", dir, l_idle);
			
		dwError = GetLastError();
		FindClose(hFind);
		/*如果出现其他异常情况，输出错误信息*/
		if (dwError != ERROR_NO_MORE_FILES) 
		{
			printf ("FindNextFile error. Error is %u\n", dwError);		
		}
	}	
}


/************************************时间处理函数********************************************/

void ftime(FILETIME filetime)
{
	SYSTEMTIME systemtime;
	/*Win32时间的低32位*/
	if (filetime.dwLowDateTime == -1)
	{
		wprintf(L"Never Expires ");
	}
	else
	{   
		/*将UTC(Universal Time Coordinated)文件时间转换成本地文件时间*/
		if (FileTimeToLocalFileTime(&filetime, &filetime) != 0) 
		{   
            /*将64位时间转换为系统时间*/
			if (FileTimeToSystemTime(&filetime, &systemtime) != 0)
			{
				char str[50];
				/*以一定格式输出时间*/
				wsprintf(str, "%d-%02d-%02d  %02d:%02d", 
					systemtime.wYear, systemtime.wMonth, systemtime.wDay,
					systemtime.wHour, systemtime.wMinute);				
				printf("%s", str);
				
			}
			else
			{   /*wprintf输出UNICODE字符*/
				wprintf(L"FileTimeToSystemTime failed ");
			}
			
		}
		else
		{
			wprintf(L"FileTimeToLocalFileTime failed ");
		}
	}	
}


/**************************************获取系统进程命令********************************************/

void GetProcessList()
{
	HANDLE  hProcessSnap = NULL;
	PROCESSENTRY32 pe32 = {0};
	/*对系统中进程进行拍照*/
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if( hProcessSnap == INVALID_HANDLE_VALUE)
		printf("\nCreateToolhelp32Snapshot() failed:%d", GetLastError ());
	
	/*使用前要填充结构大小*/    
	pe32.dwSize = sizeof(PROCESSENTRY32);
	/*列出进程*/
	if( Process32First (hProcessSnap, &pe32) )
	{
		DWORD dwPriorityClass;
		printf("\n优先级\t\t进程ID\t\t线程\t\t进程名\n");
		do{
			HANDLE hProcess;
			hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
			dwPriorityClass = GetPriorityClass (hProcess);
			CloseHandle(hProcess);
			/*输出结果*/
			printf("%d\t", pe32.pcPriClassBase);
			printf("\t%d\t", pe32.th32ProcessID);
			printf("\t%d\t", pe32.cntThreads);
            printf("\t%s\n", pe32.szExeFile);
		}
		while(Process32Next (hProcessSnap, &pe32));
	}
	else
		printf("\nProcess32finst() failed:%d", GetLastError ()); 
	CloseHandle (hProcessSnap);
}


/************************************history命令*******************************************/

void add_history(char *inputcmd)
{   
	/*end前移一位*/
	envhis.end = (envhis.end + 1) % HISNUM;
	/*end和start指向同一数组*/
	if(envhis.end == envhis.start)
	{
		/*start前移一位*/
		envhis.start = (envhis.start+1) % HISNUM;
	}
	/*将命令存入end指向的数组中*/
	strcpy(envhis.his_cmd[envhis.end], inputcmd);	
}


/************************************显示history命令********************************************/

void history_cmd()
{
	// TODO: 注意，测试中history命令有问题，请大家注意修改。
	int i, j = 1;
	/*循环数组为空，什么也不做*/
	if(envhis.start == envhis.end)
		;
	else if(envhis.start < envhis.end){
		/*显示history命令数组中start+1到end的命令*/
		for(i = envhis.start + 1;i <= envhis.end; i++)
		{
			printf("%d\t%s\n", j, envhis.his_cmd[i]);
			j++;
		}
	}else {
		/*显示history命令数组中start+1到HISNUM-1的命令*/
		for(i = envhis.start + 1;i < HISNUM; i++)
		{
			printf("%d\t%s\n", j, envhis.his_cmd[i]);
			j++;
		}
		/*显示history命令数组中0到end+1的命令*/
		for (i = 0; i <= envhis.end+1; i++)
		{
			printf("%d\t%s\n", j, envhis.his_cmd[i]);
			j++;
		}
	}
}


/***********************************创建进程命令*******************************************/

HANDLE process(int bg, char appName[])
{

	/*初始化进程相关信息*/
	STARTUPINFO si;
	PROCESS_INFORMATION   pi; 
	/*用于版本控制*/
	si.cb = sizeof(si);
	GetStartupInfo(&si);
	/*擦去pi的内容*/
	ZeroMemory(&pi, sizeof(pi));
	/*前台进程*/
	if(bg == 0)
	{  	
		/*设置钩子，捕捉ctrl+c命令，收到即结束进程*/		
		if(SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler,TRUE) == FALSE)
		{
			printf("Unable to install handler!\n");
				return NULL;
		} 
		/*调用进程相关程序，此处调用一个自己编写的程序,最好有控制台输出，注意路径正确*/
		CreateProcess(NULL,appName , NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
		//pid_for_bg=pi.dwProcessId;
		return pi.hProcess;
		
	}
	/*后台进程*/
	else
	{
		/*设置进程窗口选项*/
		si.dwFlags = STARTF_USESHOWWINDOW; 
		/*隐藏窗口*/
		si.wShowWindow = SW_HIDE;
		CreateProcess(NULL, appName, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);	
		//pid_for_fg=pi.dwProcessId;
		return pi.hProcess;
	}
}


/***********************************kill进程命令*******************************************/

BOOL killProcess(char *pid)
{
	int id, i;
	DWORD   dwExitStatus;
	HANDLE hprocess;
	id = atoi(pid);
	hprocess = OpenProcess(PROCESS_TERMINATE, FALSE, id);
	GetExitCodeProcess(hprocess, &dwExitStatus); 
	if(i == TerminateProcess(hprocess, dwExitStatus))
		return TRUE;
	else 
		return FALSE;
}


/***********************************回调函数*******************************************/

BOOL WINAPI ConsoleHandler(DWORD CEvent)
{

	switch(CEvent)
	{
		
	case CTRL_C_EVENT:                        /*由系统处理事件，包括ctrl+c等*/	
		{
			printf("....\n");
			PulseEvent(wait_sig[1]);  
		break;
		}
    case CTRL_BREAK_EVENT:
	    break;
	case CTRL_CLOSE_EVENT:		
		break;
	case CTRL_LOGOFF_EVENT:		
		break;
	case CTRL_SHUTDOWN_EVENT:		
		break;
	}
	return TRUE;
}

void transfer(unsigned long size)
{
	char c[11];
	char queue[11];
    int number=0;
	int i=0,j=0;
	if(size==0)
    {
		printf("\t\t%9lu",size);
		return;
	}
    while(size!=0)
	{
        queue[++number]=(char)(size%10+48);
		size/=10;
		if(number%3==0)
			queue[++number]=',';
	}
	if(queue[number]==',')
		number--;
    for(i=number;i>=1;i--)
	{
		c[j]=queue[i];
		j++;
	}
	c[j]='\0';
	printf("\t\t%9s",c);
}

/***********************************显示帮助*******************************************/

void help()
{
// TODO: 添加必要的注意帮助信息
	printf("cd:切换当前目录。\n输入形式：cd [drive:][path](cd C:\\temp)  \n注：cd命令以空格为分隔符，区分命令和参数。\n\n");
    printf("dir:显示目录中的文件和子文件列表。\n输入形式：dir [drive:][path](dir C:\\temp) \n注：dir命令以空格为分隔符，区分命令和参数。\n\n");
	printf("tasklist:显示系统中当前的进程信息。\n输入形式：tasklist\n\n");
	printf("fp:创建进程并在前台执行。\n输入形式：fp\n\n");
	printf("bg:创建进程并在后台执行。\n输入形式：bg\n\n");
	printf("taskkill:终止进程。\n输入形式：taskkill [pid]\n注：taskkill命令以空格为分隔符，pid为进程id。\n\n");
	printf("history:显示历史命令。\n输入形式：history\n\n");
	printf("exit:退出\n输入形式：exit\n\n");
}

/***********************************pipe*******************************************/
void pipe(char appName1[],char appName2[])
{
    HANDLE hRead1,hRead2,hWrite1,hWrite2;
	STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    char szReadBuf[100];
    DWORD nReadNum;
    BOOL bRet;
	HANDLE hTemp=GetStdHandle(STD_OUTPUT_HANDLE);

	SECURITY_ATTRIBUTES sa;
	sa.bInheritHandle=TRUE;    //必须为TRUE，父进程的读写句柄可以被子进程继承
    sa.lpSecurityDescriptor=NULL;
    sa.nLength=sizeof(SECURITY_ATTRIBUTES);

	//创建匿名管道
	bRet = CreatePipe(&hRead1,&hWrite1,&sa,0);
	bRet = CreatePipe(&hRead2,&hWrite2,&sa,0);

	//获取STARTUPINFO结构体信息
    GetStartupInfo( &si );
    //设置STARTUPINFO的标准输出到管道入口，要使标准输入输出有效，必须指定STARTF_USESTDHANDLES
    si.dwFlags=STARTF_USESTDHANDLES;
    si.hStdOutput=hWrite1;
    si.hStdError=hWrite1;

	//创建子进程
    bRet = CreateProcess( NULL, appName1, NULL, NULL, TRUE,0, NULL, NULL, &si, &pi);
    
	CloseHandle( hWrite1 );

	GetStartupInfo( &si );
	si.dwFlags=STARTF_USESTDHANDLES;
    si.hStdInput=hRead1;
	si.hStdOutput=hWrite2;
    si.hStdError=hWrite2;

	bRet = CreateProcess( NULL, appName2, NULL, NULL, TRUE,0, NULL, NULL, &si, &pi);

	CloseHandle( hWrite2 );

	//读取管道中的数据
	
    while ( ReadFile( hRead2, szReadBuf, 100, &nReadNum, NULL) )
    {
        szReadBuf[nReadNum] = '\0';
        printf( "%s\n", szReadBuf);
    }
	return;
}


