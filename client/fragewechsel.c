/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Client
 *
 * fragewechsel.c: Implementierung des Fragewechsel-Threads
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>

#include "common/rfc.h"
#include "common/util.h"
#include "fragewechsel.h"

static int questionFlag = 0;
static time_t questionTime = 0;
static pthread_mutex_t questionMutex = PTHREAD_MUTEX_INITIALIZER;

void requestNewQuestion(int seconds) {
    debugPrint("Marking QuestionThread for update in %ds...", seconds);
    pthread_mutex_lock(&questionMutex);
    if (questionFlag != 0)
        debugPrint("!Warning!: double question request!");
    questionFlag = 1;
    questionTime = time(NULL) + seconds;
    pthread_mutex_unlock(&questionMutex);
}

static void sendQuestionRequest(int socket) {
    debugPrint("Sending QuestionRequest to server...");
    rfc request;
    request.main.type[0] = 'Q';
    request.main.type[1] = 'R';
    request.main.type[2] = 'Q';
    request.main.length = htons(0);
    if (send(socket, &request.main, RFC_BASE_SIZE, 0) == -1) {
        errnoPrint("send");
    }
}

void *questionThread(void *arg) {
    int socket = *((int *)arg);
    while (1) {
        pthread_mutex_lock(&questionMutex);
        int qf = questionFlag;
        time_t qt = questionTime;
        pthread_mutex_unlock(&questionMutex);

        if ((qf > 0) && (time(NULL) >= qt)) {
            pthread_mutex_lock(&questionMutex);
            questionFlag = 0;
            questionTime = 0;
            pthread_mutex_unlock(&questionMutex);
            sendQuestionRequest(socket);
        }

        loopsleep();
    }
    return NULL;
}

