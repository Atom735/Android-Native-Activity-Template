#ifndef H_TYPES_DOT_H
#define H_TYPES_DOT_H

#include <stdint.h>
#include <pthread.h>

#define CONST const
typedef void            VOID,   *PVOID;
typedef int             INT,    *PINT,      BOOL,   *PBOOL;
typedef unsigned int    UINT,   *PUINT;
typedef int64_t         INT64,  *PINT64;
typedef uint64_t        UINT64, *PUINT64;
typedef int32_t         INT32,  *PINT32;
typedef uint32_t        UINT32, *PUINT32;
typedef int16_t         INT16,  *PINT16;
typedef uint16_t        UINT16, *PUINT16;
typedef int8_t          INT8,   *PINT8;
typedef uint8_t         UINT8,  *PUINT8,    BYTE,   *PBYTE;
typedef float           FLOAT,  *PFLOAT;
typedef size_t          SIZE_T, *PSIZE_T;

typedef char            CHAR,   *PCHAR,     *PSTR;
typedef const char                          *PCSTR;

#include <android/input.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/configuration.h>
#include <android/looper.h>
#include <android/rect.h>

typedef AInputEvent     ST_A_INPUT_EVENT,       *PST_A_INPUT_EVENT;
typedef AInputQueue     ST_A_INPUT_QUEUE,       *PST_A_INPUT_QUEUE;
typedef ANativeActivity ST_A_NATIVE_ACTIVITY,   *PST_A_NATIVE_ACTIVITY;
typedef ANativeWindow   ST_A_NATIVE_WINDOW,     *PST_A_NATIVE_WINDOW;
typedef AConfiguration  ST_A_CONFIGURATION,     *PST_A_CONFIGURATION;
typedef ALooper         ST_A_LOOPER,            *PST_A_LOOPER;
typedef ARect           ST_A_RECT,              *PST_A_RECT; 

typedef struct tagST_AAPP       ST_AAPP,        *PST_AAPP;
// typedef struct tagST_APOOLSRC   ST_APOOLSRC,    *PST_APOOLSRC;
// struct tagST_APOOLSRC {
//     INT32       nId;
//     PST_AAPP    pAApp;
//     VOID        (*prProc)(PST_AAPP in_pAApp, PST_APOOLSRC in_pAPoolSrc);
// };
struct tagST_AAPP {
    PVOID       pUserData;

    PVOID       pSavedState;
    SIZE_T      nSavedStateSize;

    INT         iActivityState;
    INT         iDestroyRequested;
    BOOL        bAnimating;

    VOID        (*prAppCom)(PST_AAPP in_pAApp, INT32 in_iCom);
    INT32       (*prOnInputEvent)(PST_AAPP in_pAApp, PST_A_INPUT_EVENT in_pInputEvent);

    PST_A_NATIVE_ACTIVITY   pNativeActivity;
    PST_A_NATIVE_WINDOW     pNativeWindow;
    PST_A_CONFIGURATION     pConfiguration;
    PST_A_LOOPER            pLooper;
    PST_A_INPUT_QUEUE       pInputQueue;
    ST_A_RECT               rcContentRect;
    struct {
        INT     iMsgRead;
        INT     iMsgWrite;

        BOOL    bRunning;
        BOOL    bSaved;
        BOOL    bDestroyed;
        BOOL    bRedrawNeeded;

        pthread_mutex_t     Mutex;
        pthread_cond_t      Cond;
        pthread_t           Thread;

        // ST_APOOLSRC         apsrcCom;
        // ST_APOOLSRC         apsrcInput;

        PST_A_INPUT_QUEUE   pInputQueue;
        PST_A_NATIVE_WINDOW pNativeWindow;
        ST_A_RECT           rcContentRect;
    } protected;
};


enum {
    // Команда из основного потока: изменился AInputQueue. После обработки этой команды android_app->inputQueue будет обновляться до новой очереди (или NULL).
    DE_APPCOM_INPUT_CHANGED,
    // Команда из основного потока: новый ANativeWindow готов к использованию. После получения этой команды окно android_app->window будет содержать новую поверхность окна.
    DE_APPCOM_WINDOW_INIT,
    // Команда из основного потока: существующая ANativeWindow должна быть завершена. После получения этой команды окно android_app->window все еще содержит существующее окно; После вызова android_app_exec_cmd будет установлено значение NULL.
    DE_APPCOM_WINDOW_TERM,
    // Команда из основного потока: текущий ANativeWindow был изменен. Требуется, перерисовка с новым размером окна.
    DE_APPCOM_WINDOW_RESIZED,
    // Команда из основного потока: системе необходимо, чтобы текущий ANativeWindow был перерисован. Вы должны перерисовать окно, прежде чем передать это android_app_exec_cmd() для того, чтобы избежать переходных глюков рисования.
    DE_APPCOM_WINDOW_REDRAW_NEEDED,
    // Команда из основного потока: изменилась область содержимого окна, например, из отображаемого или скрытого окна мягкого ввода. Вы можете найти новый контент rect в android_app::contentRect.
    DE_APPCOM_CONTENT_RECT_CHANGED,
    // Команда из основного потока: окно активности приложения получило фокус ввода.
    DE_APPCOM_FOCUS_GAINED,
    // Команда из основного потока: окно активности приложения потеряло фокус ввода.
    DE_APPCOM_FOCUS_LOST,
    // Команда из основного потока: изменилась конфигурация текущего устройства.
    DE_APPCOM_CONFIG_CHANGED,
    // Команда из основного потока: система работает c малым количсетвом памяти. Попытайтесь уменьшить использование памяти.
    DE_APPCOM_LOW_MEMORY,
    // Команда из основного потока: активность приложения стартовала.
    DE_APPCOM_START,
    // Команда из основного потока: активность приложения продолжилась.
    DE_APPCOM_RESUME,
    // Команда из основного потока: активность приложения приостановлена.
    DE_APPCOM_PAUSE,
    // Команда из основного потока: активность приложения остановилась.
    DE_APPCOM_STOP,
    // Команда из основного потока: активность приложения уничтожается и ждет, пока поток приложения не будет очищен и не выйдет, прежде чем продолжить.
    DE_APPCOM_DESTROY,
    // Команда из основного потока: приложение должно сгенерировать новое сохраненное состояние для себя, чтобы восстановиться позже, если это необходимо. Если вы сохранили состояние, выделите его с помощью malloc и поместите его в android_app.savedState с размером в android_app.savedStateSize. Позднее выделенная память сама освободится для вас.
    DE_APPCOM_SAVE_STATE,
};
enum {
    // Идентификатор данных Looper команд, поступающих из основного потока приложения, который возвращается как идентификатор из ALooper_pollOnce(). Данные для этого идентификатора являются указателями на структуру android_poll_source. Они могут быть получены и обработаны с помощью android_app_read_cmd() и android_app_exec_cmd().
    DE_LOOPER_ID_MAIN = 1,
    // Идентификатор данных Looper из событий, поступающих из окна AInputQueue окна приложения, который возвращается как идентификатор из ALooper_pollOnce(). Данные для этого идентификатора являются указателями на структуру android_poll_source. Они могут быть прочитаны через объект inputQueue из android_app.
    DE_LOOPER_ID_INPUT = 2,
    // Начало определяемого пользователем ALooper идентификаторов.
    DE_LOOPER_ID_USER = 3,
};

// // Вызовите, когда ALooper_pollAll() возвращает LOOPER_ID_MAIN, прочитав следующее сообщение команды приложения.
// int8_t android_app_read_cmd(struct android_app* android_app);

// // Вызовите команду, возвращенную android_app_read_cmd(), чтобы выполнить первоначальную предварительную обработку данной команды. Вы можете выполнить свои собственные действия для команды после вызова этой функции.
// void android_app_pre_exec_cmd(struct android_app* android_app, int8_t cmd);

// // Вызовите команду, возвращенную android_app_read_cmd(), чтобы выполнить окончательную постобработку данной команды. Вы должны были выполнить свои собственные действия для команды перед вызовом этой функции.
// void android_app_post_exec_cmd(struct android_app* android_app, int8_t cmd);

// // Dummy, которая использовалась для предотвращения компоновщика ссылок для удаления дескриптора приложения. Больше не нужно, поскольку __attribute__((visibility("default"))) делает это для нас.
// __attribute__((
//     deprecated("Calls to app_dummy are no longer necessary. See "
//                "https://github.com/android-ndk/ndk/issues/381."))) void
// app_dummy();

// // Это функция, код приложения должен реализовать, представляющий основной вход в приложение.
// extern void android_main(struct android_app* app);


#endif