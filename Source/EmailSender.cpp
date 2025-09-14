/*
 *  Copyright (c) 2014, Oculus VR, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant 
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "../include/RakNet/NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_EmailSender==1 && _RAKNET_SUPPORT_TCPInterface==1 && _RAKNET_SUPPORT_FileOperations==1

// Useful sites
// http://www.faqs.org\rfcs\rfc2821.html
// http://www2.rad.com\networks/1995/mime/examples.htm

#include "../include/RakNet/EmailSender.h"
#include "../include/RakNet/TCPInterface.h"
#include "../include/RakNet/GetTime.h"
#include "../include/RakNet/Rand.h"
#include "../include/RakNet/FileList.h"
#include "../include/RakNet/BitStream.h"
#include "../include/RakNet/Base64Encoder.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include "../include/RakNet/RakSleep.h"

using namespace RakNet;

STATIC_FACTORY_DEFINITIONS(EmailSender,EmailSender);

const char *EmailSender::Send(const char *hostAddress, unsigned short hostPort, const char *sender, const char *recipient, const char *senderName, const char *recipientName, const char *subject, const char *body, FileList *attachedFiles, bool doPrintf, const char *password)
{
    RakNet::Packet *packet;
    char query[1024];
    TCPInterface tcpInterface;
    SystemAddress emailServer;
    if (tcpInterface.Start(0, 0)==false)
        return "Unknown error starting TCP";
    emailServer=tcpInterface.Connect(hostAddress, hostPort,true);
    if (emailServer==UNASSIGNED_SYSTEM_ADDRESS)
        return "Failed to connect to host";
#if OPEN_SSL_CLIENT_SUPPORT == 1
    tcpInterface.StartSSLClient(emailServer);
#endif
    RakNet::TimeMS timeoutTime = RakNet::GetTimeMS()+3000;
    packet=nullptr;
    while (RakNet::GetTimeMS() < timeoutTime)
    {
        packet = tcpInterface.Receive();
        if (packet)
        {
            if (doPrintf)
            {
                RAKNET_DEBUG_PRINTF("%s", packet->data);
                tcpInterface.DeallocatePacket(packet);
            }
            break;
        }
        RakSleep(250);
    }

    if (packet==nullptr)
        return "Timeout while waiting for initial data from server.";

    tcpInterface.Send("EHLO\r\n", 6, emailServer,false);

    auto send_fmt = [&](const char* fmt, ...) -> const char* {
        va_list ap;
        va_start(ap, fmt);
        int n = std::vsnprintf(query, sizeof(query), fmt, ap);
        va_end(ap);
        if (n < 0) return "Formatting error";

        auto to_send = (unsigned int) ((n >= (int)sizeof(query)) ? sizeof(query) - 1u : (unsigned int)n);
        tcpInterface.Send(query, to_send, emailServer, false);
        return nullptr;
    };

    const char *response;
    bool authenticate=false;
#ifdef _MSC_VER
#pragma warning(disable:4127)   // conditional expression is constant
#endif
    while (true)
    {
        response=GetResponse(&tcpInterface, emailServer, doPrintf);

        if (response!=0 && std::strcmp(response, "AUTHENTICATE")==0)
        {
            authenticate=true;
            break;
        }

        if (response!=0 && std::strcmp(response, "CONTINUE")!=0)
            return response;

        if (response==0)
            break;
    }

    if (authenticate)
    {
        if (const char* err = send_fmt("EHLO %s\r\n", sender ? sender : "")) return err;
        response=GetResponse(&tcpInterface, emailServer, doPrintf);
        if (response!=0)
            return response;
        if (password==0)
            return "Password needed";
        char *outputData = RakNet::OP_NEW_ARRAY<char >((const int) (std::strlen(sender?sender:"")+std::strlen(password)+2)*3, _FILE_AND_LINE_ );
        RakNet::BitStream bs;
        char zero=0;
        bs.Write(&zero,1);
        if (sender) bs.Write(sender,(const unsigned int)std::strlen(sender));
        bs.Write(&zero,1);
        bs.Write(password,(const unsigned int)std::strlen(password));
        bs.Write(&zero,1);
        Base64Encoding((const unsigned char*)bs.GetData(), bs.GetNumberOfBytesUsed(), outputData);
        if (const char* err = send_fmt("AUTH PLAIN %s", outputData)) return err;
        response=GetResponse(&tcpInterface, emailServer, doPrintf);
        if (response!=0)
            return response;
    }

    if (sender)
    { if (const char* err = send_fmt("MAIL From: <%s>\r\n", sender)) return err; }
    else
    { if (const char* err = send_fmt("MAIL From: <>\r\n")) return err; }
    response=GetResponse(&tcpInterface, emailServer, doPrintf);
    if (response!=nullptr)
        return response;

    if (recipient)
    { if (const char* err = send_fmt("RCPT TO: <%s>\r\n", recipient)) return err; }
    else
    { if (const char* err = send_fmt("RCPT TO: <>\r\n")) return err; }
    response=GetResponse(&tcpInterface, emailServer, doPrintf);
    if (response!=nullptr)
        return response;

    tcpInterface.Send("DATA\r\n", (unsigned int)(sizeof("DATA\r\n")-1), emailServer,false);

    response=GetResponse(&tcpInterface, emailServer, doPrintf);
    if (response!=nullptr)
        return response;

    if (subject)
    { if (const char* err = send_fmt("Subject: %s\r\n", subject)) return err; }
    if (senderName)
    { if (const char* err = send_fmt("From: %s\r\n", senderName)) return err; }
    if (recipientName)
    { if (const char* err = send_fmt("To: %s\r\n", recipientName)) return err; }

    const int boundarySize=60;
    char boundary[boundarySize+1];
    int i,j;
    if (attachedFiles && attachedFiles->fileList.Size())
    {
        rakNetRandom.SeedMT((unsigned int) RakNet::GetTimeMS());
        for (i=0; i < boundarySize; i++)
            boundary[i]=Base64Map()[rakNetRandom.RandomMT()%64];
        boundary[boundarySize]=0;
    }

    if (const char* err = send_fmt("MIME-version: 1.0\r\n")) return err;

    if (attachedFiles && attachedFiles->fileList.Size())
    {
        if (const char* err = send_fmt("Content-type: multipart/mixed; BOUNDARY=\"%s\"\r\n\r\n", boundary)) return err;
        if (const char* err = send_fmt("This is a multi-part message in MIME format.\r\n\r\n--%s\r\n", boundary)) return err;
    }

    if (const char* err = send_fmt("Content-Type: text/plain; charset=\"US-ASCII\"\r\n\r\n")) return err;

    char *newBody;
    int bodyLength = (int)std::strlen(body);
    newBody = (char*) rakMalloc_Ex( (size_t)bodyLength*3, _FILE_AND_LINE_ );
    if (bodyLength>=0)
        newBody[0]=body[0];
    for (i=1, j=1; i < bodyLength; i++)
    {
        if (i < bodyLength-2 &&
            body[i-1]=='\n' &&
            body[i+0]=='.' &&
            body[i+1]=='\r' &&
            body[i+2]=='\n')
        {
            newBody[j++]='.';
            newBody[j++]='.';
            newBody[j++]='\r';
            newBody[j++]='\n';
            i+=2;
        }
        else if (i <= bodyLength-3 &&
                 body[i-1]=='\n' &&
                 body[i+0]=='.' &&
                 body[i+1]=='.' &&
                 body[i+2]=='\r' &&
                 body[i+3]=='\n')
        {
            newBody[j++]='.';
            newBody[j++]='.';
            newBody[j++]='.';
            newBody[j++]='\r';
            newBody[j++]='\n';
            i+=3;
        }
        else if (i < bodyLength-1 &&
                 body[i-1]=='\n' &&
                 body[i+0]=='.' &&
                 body[i+1]=='\n')
        {
            newBody[j++]='.';
            newBody[j++]='.';
            newBody[j++]='\r';
            newBody[j++]='\n';
            i+=1;
        }
        else if (i <= bodyLength-2 &&
                 body[i-1]=='\n' &&
                 body[i+0]=='.' &&
                 body[i+1]=='.' &&
                 body[i+2]=='\n')
        {
            newBody[j++]='.';
            newBody[j++]='.';
            newBody[j++]='.';
            newBody[j++]='\r';
            newBody[j++]='\n';
            i+=2;
        }
        else
            newBody[j++]=body[i];
    }

    newBody[j++]='\r';
    newBody[j++]='\n';
    tcpInterface.Send(newBody, j, emailServer,false);

    rakFree_Ex(newBody, _FILE_AND_LINE_ );
    int outputOffset;

    if (attachedFiles && attachedFiles->fileList.Size())
    {
        for (i=0; i < (int) attachedFiles->fileList.Size(); i++)
        {
            if (const char* err = send_fmt("\r\n--%s\r\n", boundary)) return err;

            if (const char* err = send_fmt(
                    "Content-Type: APPLICATION/Octet-Stream; SizeOnDisk=%i; name=\"%s\"\r\n"
                    "Content-Transfer-Encoding: BASE64\r\n"
                    "Content-Description: %s\r\n\r\n",
                    attachedFiles->fileList[i].dataLengthBytes,
                    attachedFiles->fileList[i].filename.C_String(),
                    attachedFiles->fileList[i].filename.C_String()))
                return err;

            char* newBodyFile = (char*) rakMalloc_Ex( (size_t) (attachedFiles->fileList[i].dataLengthBytes*3)/2, _FILE_AND_LINE_ );
            outputOffset = Base64Encoding((const unsigned char*) attachedFiles->fileList[i].data, (int) attachedFiles->fileList[i].dataLengthBytes, newBodyFile);
            tcpInterface.Send(newBodyFile, outputOffset, emailServer,false);
            rakFree_Ex(newBodyFile, _FILE_AND_LINE_ );
        }

        if (const char* err = send_fmt("\r\n--%s--\r\n", boundary)) return err;
    }

    tcpInterface.Send("\r\n.\r\n", (unsigned int)(sizeof("\r\n.\r\n")-1), emailServer,false);
    response=GetResponse(&tcpInterface, emailServer, doPrintf);
    if (response!=nullptr)
        return response;

    tcpInterface.Send("QUIT\r\n", (unsigned int)(sizeof("QUIT\r\n")-1), emailServer,false);

    RakSleep(30);
    if (doPrintf)
    {
        packet = tcpInterface.Receive();
        while (packet)
        {
            RAKNET_DEBUG_PRINTF("%s", packet->data);
            tcpInterface.DeallocatePacket(packet);
            packet = tcpInterface.Receive();
        }
    }
    tcpInterface.Stop();
    return nullptr;
}

const char *EmailSender::GetResponse(TCPInterface *tcpInterface, const SystemAddress &emailServer, bool doPrintf)
{
	RakNet::Packet *packet;
	RakNet::TimeMS timeout;
	timeout=RakNet::GetTimeMS()+5000;
#ifdef _MSC_VER
	#pragma warning( disable : 4127 ) // warning C4127: conditional expression is constant
#endif
	while (1)
	{
		if (tcpInterface->HasLostConnection()==emailServer)
			return "Connection to server lost.";
		packet = tcpInterface->Receive();
		if (packet)
		{
			if (doPrintf)
			{
				RAKNET_DEBUG_PRINTF("%s", packet->data);
			}
#if OPEN_SSL_CLIENT_SUPPORT==1
			if (strstr((const char*)packet->data, "220"))
			{
				tcpInterface->StartSSLClient(packet->systemAddress);
				return "AUTHENTICATE"; // OK
			}
// 			if (strstr((const char*)packet->data, "250-AUTH LOGIN PLAIN"))
// 			{
// 				tcpInterface->StartSSLClient(packet->systemAddress);
// 				return "AUTHENTICATE"; // OK
// 			}
#endif
			if (strstr((const char*)packet->data, "235"))
				return 0; // Authentication accepted
			if (strstr((const char*)packet->data, "354"))
				return 0; // Go ahead
#if OPEN_SSL_CLIENT_SUPPORT==1
			if (strstr((const char*)packet->data, "250-STARTTLS"))
			{
				tcpInterface->Send("STARTTLS\r\n", (unsigned int) strlen("STARTTLS\r\n"), packet->systemAddress, false);
				return "CONTINUE";
			}
#endif
			if (strstr((const char*)packet->data, "250"))
				return 0; // OK
			if (strstr((const char*)packet->data, "550"))
				return "Failed on error code 550";
			if (strstr((const char*)packet->data, "553"))
				return "Failed on error code 553";
		}
		if (RakNet::GetTimeMS() > timeout)
			return "Timed out";
		RakSleep(100);
	}
}


#endif // _RAKNET_SUPPORT_*
