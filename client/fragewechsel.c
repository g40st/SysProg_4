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

static semaphore_t questionMutex;
static time_t questionTime = 0;

void requestNewQuestion(int seconds) {
    debugPrint("Marking QuestionThread for update in %ds...", seconds);
    questionTime = time(NULL) + seconds; // Request after seconds seconds.
    semaphorePost(questionMutex);
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
    questionMutex = semaphoreNew(0);

    while (1) {
        semaphoreWait(questionMutex);

        // If enough time passed
        if ((time(NULL) >= questionTime)) {
            // Reset the flag
            questionTime = 0;

            // Request a new question
            sendQuestionRequest(socket);
        } else {
            semaphorePost(questionMutex);
        }
    }

    semaphoreRelease(questionMutex);
    return NULL;
}

