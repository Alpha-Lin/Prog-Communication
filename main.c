/*---------------------------------- By Alpha_Lin ----------------------------------*/

//DON'T FORGET -lws2_32 OPTION TO COMPILE

#ifdef WIN32

#include <winsock2.h>
#include <windows.h>

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#endif

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "fonctions.h"

typedef struct threadConnectionArguments{
    int socket;
    FILE *fichier;
    SYSTEMTIME Time;
} threadConnectionArguments;

unsigned char choix;

void ClearConsole(void);
void confirmAndWriteTheSuccessfulConnection(FILE *fichier, SYSTEMTIME Time, char adresseIP[15]);
void *checkMessage(void *structData);//int socket, FILE *fichier, SYSTEMTIME Time);
void *sendMessage(void *structData);//int socket, FILE *fichier, SYSTEMTIME Time);

int main(int argc, char **argv)
{
    SYSTEMTIME TimeMain;

    GetLocalTime(&TimeMain);

    FILE *fichier_config = fopen("settings.ini", "r+");

    if(fichier_config == NULL)
    {
        fclose(fichier_config);
        fichier_config = fopen("settings.ini", "w+");
        fputs("IP = IPv4\nlog = yes", fichier_config);
    }

    char logName[100];

    sprintf(logName, "log-%02d.%02d.%02d.%02d.%02d.%02d.txt", TimeMain.wYear, TimeMain.wMonth, TimeMain.wDay,TimeMain.wHour, TimeMain.wMinute, TimeMain.wSecond);

    FILE *fichier_log = fopen(logName, "w");

    if(fichier_log == NULL)
        exit(EXIT_FAILURE);
    
    char parametreIP[2];
    char parametreLog[4];
    unsigned char choixMenu = 0;
    do{
        ClearConsole();
        printf("Navigation :\n\t1)Etablir une connexion\n\t2)Acceder aux parametres\n\t3)Quitter\n");

        scanf("%d", &choixMenu);

        ClearConsole();

        if(choixMenu == 1)
        {
            #ifdef WIN32
            WSADATA wsa;
            if(WSAStartup(MAKEWORD(2, 2), &wsa) < 0)
            {
                printf("Erreur lors du demarrage de WSAStartup()\n");
                GetLocalTime(&TimeMain);
                fprintf(fichier_log, "[%02d:%02d:%02d] Erreur lors du demarrage de WSAStartup()\n", TimeMain.wHour, TimeMain.wMinute, TimeMain.wSecond);
                continue;
            }
            #endif

            //créer le socket
            int network_socket = socket(AF_INET, SOCK_STREAM, 0);

            printf("Etes-vous le client ou le serveur ? (0/1) : ");
            scanf("%d", &choix);

            if(choix == 1) //serveur
            {
                struct sockaddr_in server_address;
                server_address.sin_family = AF_INET;
                server_address.sin_port = htons(54213);
                server_address.sin_addr.s_addr = INADDR_ANY;

                //liaison du port et de l'adresse IP
                bind(network_socket, (struct sockaddr*) &server_address, sizeof(server_address));

                listen(network_socket, 0);

                printf("En attente de l'interlocuteur sur le port 54213...\n");

                GetLocalTime(&TimeMain);
                
                fprintf(fichier_log, "[%02d:%02d:%02d] En attente de l'interlocuteur sur le port 54213...\n", TimeMain.wHour, TimeMain.wMinute, TimeMain.wSecond);

                struct sockaddr_in client_socket = {0};

                int sinsize = sizeof(client_socket);

                int client_socket_response = accept(network_socket, (struct sockaddr *)&client_socket, &sinsize);

                confirmAndWriteTheSuccessfulConnection(fichier_log, TimeMain, inet_ntoa(client_socket.sin_addr));

                threadConnectionArguments argsThread = {client_socket_response, fichier_log, TimeMain};

                pthread_t receptionThread, envoieThread;

                //reception et envoie des données envoyées par le serveur et le client
                pthread_create(&envoieThread, NULL, sendMessage, (void *) &argsThread);
                pthread_create(&receptionThread, NULL, checkMessage, (void *) &argsThread);

                pthread_join(receptionThread, NULL);
                pthread_join(envoieThread, NULL);

            }else if(!choix)//client
            {
                printf("Entrez l'adresse IP de votre interlocuteur : ");

                char adresseIP[15];
                scanf("%s", &adresseIP);

                printf("Etablissement de la connexion sur l'adresse IPv4 %s sur le port 54213 en cours...\n", adresseIP);

                GetLocalTime(&TimeMain);
                fprintf(fichier_log, "[%02d:%02d:%02d] Etablissement de la connexion sur l'adresse IPv4 %s sur le port 54213 en cours...\n", TimeMain.wHour, TimeMain.wMinute, TimeMain.wSecond, adresseIP);

                struct sockaddr_in server_address;
                server_address.sin_family = AF_INET;
                server_address.sin_port = htons(54213);
                server_address.sin_addr.s_addr = inet_addr(adresseIP);
                
                //démarre et vérifie la connexion
                if(connect(network_socket, (struct sockaddr *) &server_address, sizeof(server_address)) == -1)
                {
                    printf("Il y a eu une erreur pendant la connexion avec le socket\n");
                    GetLocalTime(&TimeMain);
                    fprintf(fichier_log, "[%02d:%02d:%02d] Il y a eu une erreur pendant la connexion avec le socket\n", TimeMain.wHour, TimeMain.wMinute, TimeMain.wSecond);
                }
                else
                {
                    confirmAndWriteTheSuccessfulConnection(fichier_log, TimeMain, adresseIP);

                    threadConnectionArguments argsThread = {network_socket, fichier_log, TimeMain};

                    pthread_t receptionThread, envoieThread;

                    //reception et envoie des données envoyées par le serveur et le client
                    pthread_create(&receptionThread, NULL, checkMessage, (void *) &argsThread);
                    pthread_create(&envoieThread, NULL, sendMessage, (void *) &argsThread);

                    pthread_join(receptionThread, NULL);
                    pthread_join(envoieThread, NULL);
                }
            }else
                printf("Choix incorrect .");

            //fermer le socket
            #ifdef WIN32

            closesocket(network_socket);
            WSACleanup();

            #else
            
            close(network_socket);  
            
            #endif
            
        }else if(choixMenu == 2)
        {
            unsigned char badOptionSettings = 0;

            while(1)
            {
                ClearConsole();
                unsigned char parametreChoix = 0;
                if(badOptionSettings)
                {
                    printf("Option Invalide.\n");
                    badOptionSettings = 0;
                }

                fseek(fichier_config, 8, SEEK_SET);
                fgets(parametreIP, 2, fichier_config);
                fseek(fichier_config, 17, SEEK_SET);
                fgets(parametreLog, 4, fichier_config);
                printf("Parametres :\n\tIP = IPv%s\n\tlog = %s\n\n\t1)inverser IPv4/IPv6\n\t2)activer/desactiver la conservation des logs\n\t3)Menu principal\n", parametreIP, parametreLog);
                scanf("%d", &parametreChoix);
                if(parametreChoix == 1)
                {
                    fseek(fichier_config, 8, SEEK_SET);
                    fgets(parametreIP, 2, fichier_config);
                    fseek(fichier_config, 8, SEEK_SET);
                    if(!strcmp(parametreIP, "4\0"))
                        fputs("6", fichier_config);
                    else
                        fputs("4", fichier_config);
                }else if(parametreChoix == 2)
                {
                    fseek(fichier_config, 17, SEEK_SET);
                    fgets(parametreLog, 4, fichier_config);
                    fseek(fichier_config, 17, SEEK_SET);
                    if(!strcmp(parametreLog, "yes\0"))
                        fputs("no ", fichier_config);
                    else
                        fputs("yes", fichier_config);
                }else if(parametreChoix == 3)
                {
                    ClearConsole();
                    break;
                }else
                    badOptionSettings = 1;
            }
        }
        else if(choixMenu == 3)
            break;
        else
        {
            printf("Choix incorrect veuillez recommencer.\n");
        }
    }while(choixMenu != 3);

    fclose(fichier_log);

    fseek(fichier_config, 17, SEEK_SET);
    fgets(parametreLog, 4, fichier_config);
    fseek(fichier_config, 17, SEEK_SET);
    if(!strcmp(parametreLog, "no \0"))
        remove(logName);

    fclose(fichier_config);

    return EXIT_SUCCESS;
}

void ClearConsole(void)
{
    #if defined(_WIN32) || defined(_WIN64)
        system("cls");
    #else
        system("clear");
    #endif
}

void confirmAndWriteTheSuccessfulConnection(FILE *fichier, SYSTEMTIME Time, char adresseIP[15])
{
    printf("Etablissment de la connexion reussi sur l'adresse IPv4 %s sur le port 54213.\n", adresseIP);
    GetLocalTime(&Time);
    fprintf(fichier, "[%02d:%02d:%02d] Etablissment de la connexion reussi sur l'adresse IPv4 %s sur le port 54213.\n", Time.wHour, Time.wMinute, Time.wSecond, adresseIP);
}

void *checkMessage(void *structData)//int socket, FILE *fichier, SYSTEMTIME Time)
{ 
    char response[255] = "";
    threadConnectionArguments *connectionData = structData;

    while(1){
        GetLocalTime(&connectionData -> Time);

        recv(connectionData -> socket, response, sizeof(response), 0);

        //afficher la réponse du serveur
        if(strcmp("", response) && strcmp("\n", response))
        {
            if(!strcmp("/break\n", response)){                                      //quitte si il en recoit l'ordre
                printf("Connexion interrompue, entrez /break pour retourner au menu de navigation\n");
                fprintf(connectionData -> fichier, "[%02d:%02d:%02d] Connexion interrompue, entrez /break pour retourner au menu de navigation\n", connectionData -> Time.wHour, connectionData -> Time.wMinute, connectionData -> Time.wSecond);
                send(connectionData -> socket, "/break\n", sizeof("/break\n"), 0);

                return NULL;
            }
            else if(choix)
            {
                printf("\nClient : %s", response);
                fprintf(connectionData -> fichier, "[%02d:%02d:%02d] Client : %s", connectionData -> Time.wHour, connectionData -> Time.wMinute, connectionData -> Time.wSecond, response);
            }
            else
            {
                printf("\nServeur : %s", response);
                fprintf(connectionData -> fichier, "[%02d:%02d:%02d] Serveur : %s", connectionData -> Time.wHour, connectionData -> Time.wMinute, connectionData -> Time.wSecond, response);
            }
        }
    }
}

void *sendMessage(void *structData)//int socket, FILE *fichier, SYSTEMTIME Time)
{
    char message[255];
    threadConnectionArguments *connectionData = structData;

    while(1){
        fgets(message, 255, stdin);

        GetLocalTime(&connectionData -> Time);

        fprintf(connectionData -> fichier, "[%02d:%02d:%02d] Message : %s", connectionData -> Time.wHour, connectionData -> Time.wMinute, connectionData -> Time.wSecond, message);

        send(connectionData -> socket, message, sizeof(message), 0);

        if(!strcmp("/break\n", message))//quitte si il en recoit l'ordre
        {
            printf("Connexion interrompue, retour au menu de navigation\n");
            fprintf(connectionData -> fichier, "[%02d:%02d:%02d] Connexion interrompue, retour au menu de navigation\n", connectionData -> Time.wHour, connectionData -> Time.wMinute, connectionData -> Time.wSecond);
            return NULL;
        }
    }
}
