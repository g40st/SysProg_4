/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * user.h: Header für die User-Verwaltung
 */

#ifndef USER_H
#define USER_H

/*
 * Initialize the user database to default values.
 */
void userInit(void);

/*
 * Returns the count of present users
 */
int userCount(void);

/*
 * Returns the first database slot that is not present.
 */
int userFirstFreeSlot(void);

/*
 * Get or Set the main listening socket.
 */
void userSetMainSocket(int s);
int userGetMainSocket(void);

/*
 * Get or Set the present flag of the specified slot.
 */
void userSetPresent(int index, int present);
int userGetPresent(int index);

/*
 * Get or Set the client socket of the specified slot.
 */
void userSetSocket(int index, int socket);
int userGetSocket(int index);

/*
 * Get or Set the name of the user in the specified slot.
 */
void userSetName(int index, const char *name);
const char *userGetName(int index);

/*
 * Get the score of the user in the specified slot.
 */
int userGetScore(int index);

/*
 * Increase the score of the specified user, using the given
 * point formula. timeLeft and timeout should both be in seconds.
 */
void userAddScore(int index, unsigned long timeLeft, unsigned long timeout);

/*
 * Flag to keep track of Catalog Changed Events for new users.
 * If the catalog changes, and a new user logs in afterwards, the
 * Catalog Change message needs to be relayed to him.
 */
void userSetLastCCH(int index, int cch);
int userGetLastCCH(int index);

/*
 * Get or Set the answered-state of the specified question
 * for the user in the specified slot.
 */
#define MAX_QUESTIONS 128
void userSetQuestion(int index, int question, int q);
int userGetQuestion(int index, int question);

/*
 * Returns the number of questions a user has already answered.
 */
int userCountQuestionsAnswered(int index);

/*
 * Calculates the rank of the specified user in relation to all
 * other currently present users.
 */
int userGetRank(int index);

/*
 * Get or Set the timeout value for the specified user.
 */
void userSetLastTimeout(int index, int timeout);
int userGetLastTimeout(int index);

/*
 * Helper function to wait for activity on a present users
 * socket. Returns 0 on timeout, -1 on error, or 1.
 */
int waitForSocket(int id, int timeout);

#endif

