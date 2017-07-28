#include "h_types.h"
#include "h_log.h"

#include <jni.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>

#include <android/log.h>

static VOID ANA_rOnDestroy(PST_A_NATIVE_ACTIVITY in_pNativeActivity);
static VOID ANA_rOnStart(PST_A_NATIVE_ACTIVITY in_pNativeActivity);
static VOID ANA_rOnResume(PST_A_NATIVE_ACTIVITY in_pNativeActivity);
static VOID ANA_rOnPause(PST_A_NATIVE_ACTIVITY in_pNativeActivity);
static VOID ANA_rOnStop(PST_A_NATIVE_ACTIVITY in_pNativeActivity);

static VOID ANA_rOnWindowFocusChanged(PST_A_NATIVE_ACTIVITY in_pNativeActivity, BOOL in_bFocused);
static VOID ANA_rOnLowMemory(PST_A_NATIVE_ACTIVITY in_pNativeActivity);
static VOID ANA_rOnConfigurationChanged(PST_A_NATIVE_ACTIVITY in_pNativeActivity);
static VOID ANA_rOnNativeWindowCreated(PST_A_NATIVE_ACTIVITY in_pNativeActivity, PST_A_NATIVE_WINDOW in_pNativeWindow);
static VOID ANA_rOnNativeWindowDestroyed(PST_A_NATIVE_ACTIVITY in_pNativeActivity, PST_A_NATIVE_WINDOW in_pNativeWindow);
static VOID ANA_rOnInputQueueCreated(PST_A_NATIVE_ACTIVITY in_pNativeActivity, PST_A_INPUT_QUEUE in_pInputQueue);
static VOID ANA_rOnInputQueueDestroyed(PST_A_NATIVE_ACTIVITY in_pNativeActivity, PST_A_INPUT_QUEUE in_pInputQueue);
static PVOID ANA_rOnSaveInstanceState(PST_A_NATIVE_ACTIVITY in_pNativeActivity, PSIZE_T out_nLen);

static VOID AAPP_rSetActivityState(PST_AAPP in_pAApp, INT in_iCom);
static VOID AAPP_rSetInputQueue(PST_AAPP in_pAApp, PST_A_INPUT_QUEUE in_pInputQueue);
static VOID AAPP_rSetNativeWindow(PST_AAPP in_pAApp, PST_A_NATIVE_WINDOW in_pNativeWindow);
static VOID AAPP_rWriteCom(PST_AAPP in_pAApp, BYTE in_iCom);
static VOID AAPP_rFreeSavedState(PST_AAPP in_pAApp);

static VOID LOG_rPrintConfiguration(PST_A_CONFIGURATION in_pConfiguration);
static PVOID MAIN_rAppEntry(PVOID in_pData);
static VOID MAIN_rProcInput(PST_AAPP in_pAApp);
static VOID MAIN_rProcCom(PST_AAPP in_pAApp);



JNIEXPORT
VOID ANativeActivity_onCreate(PST_A_NATIVE_ACTIVITY in_pNativeActivity,
    PVOID in_pSavedState, SIZE_T in_nSavedStateSize) {
    LOG_V("Creating: %p\n", in_pNativeActivity);

    // Создаём приложение в памяти
    PST_AAPP l_pAApp = (PST_AAPP)malloc(sizeof(ST_AAPP));
    memset(l_pAApp, 0, sizeof(ST_AAPP));
    l_pAApp->pNativeActivity = in_pNativeActivity;

    // Проверяем, существуют ли сохранённые данные
    if (in_pSavedState) {
        l_pAApp->pSavedState = malloc(l_pAApp->nSavedStateSize = in_nSavedStateSize);
        memcpy(l_pAApp->pSavedState, in_pSavedState, in_nSavedStateSize);
    }
    // Подготавливаем межпотоковую очередь сообщений.
    {
        INT msgpipe[2];
        pthread_attr_t attr; 
    
        if (pipe(msgpipe)) {
            LOG_F("could not create pipe: %s", strerror(errno));
            in_pNativeActivity->instance = NULL;
            if(l_pAApp->pSavedState) {free(l_pAApp->pSavedState);}
            free(l_pAApp);
            return;
        }
        l_pAApp->protected.iMsgRead = msgpipe[0];
        l_pAApp->protected.iMsgWrite = msgpipe[1];
    
        pthread_mutex_init(&l_pAApp->protected.Mutex, NULL);
        pthread_cond_init(&l_pAApp->protected.Cond, NULL);
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        // Запускаем поток приложения
        pthread_create(&l_pAApp->protected.Thread, &attr, MAIN_rAppEntry, l_pAApp);
        // Ждём инициализации потока
        pthread_mutex_lock(&l_pAApp->protected.Mutex);
        while (!l_pAApp->protected.bRunning) {
            pthread_cond_wait(&l_pAApp->protected.Cond, &l_pAApp->protected.Mutex);
        }
        pthread_mutex_unlock(&l_pAApp->protected.Mutex);
    }

    // Устанавливаем колбеки
    in_pNativeActivity->callbacks->onDestroy = ANA_rOnDestroy;
    in_pNativeActivity->callbacks->onStart = ANA_rOnStart;
    in_pNativeActivity->callbacks->onResume = ANA_rOnResume;
    in_pNativeActivity->callbacks->onPause = ANA_rOnPause;
    in_pNativeActivity->callbacks->onStop = ANA_rOnStop;
    in_pNativeActivity->callbacks->onConfigurationChanged = ANA_rOnConfigurationChanged;
    in_pNativeActivity->callbacks->onLowMemory = ANA_rOnLowMemory;
    in_pNativeActivity->callbacks->onWindowFocusChanged = ANA_rOnWindowFocusChanged;
    in_pNativeActivity->callbacks->onNativeWindowCreated = ANA_rOnNativeWindowCreated;
    in_pNativeActivity->callbacks->onNativeWindowDestroyed = ANA_rOnNativeWindowDestroyed;
    in_pNativeActivity->callbacks->onInputQueueCreated = ANA_rOnInputQueueCreated;
    in_pNativeActivity->callbacks->onInputQueueDestroyed = ANA_rOnInputQueueDestroyed;
    in_pNativeActivity->callbacks->onSaveInstanceState = ANA_rOnSaveInstanceState;
    in_pNativeActivity->instance = l_pAApp;
}

static VOID ANA_rOnDestroy(PST_A_NATIVE_ACTIVITY in_pNativeActivity) {
    LOG_V("Destroy: %p\n", in_pNativeActivity);
    PST_AAPP l_pAApp = (PST_AAPP)in_pNativeActivity->instance;
    pthread_mutex_lock(&l_pAApp->protected.Mutex);
    AAPP_rWriteCom(l_pAApp, DE_APPCOM_DESTROY);
    // Ждём сигнала от потока приложения, что оно завершило работу...
    while(!l_pAApp->protected.bDestroyed) {
        pthread_cond_wait(&l_pAApp->protected.Cond, &l_pAApp->protected.Mutex);
    }
    pthread_mutex_unlock(&l_pAApp->protected.Mutex);
    // Высвобождаем ресурсы
    close(l_pAApp->protected.iMsgRead);
    close(l_pAApp->protected.iMsgWrite);
    pthread_cond_destroy(&l_pAApp->protected.Cond);
    pthread_mutex_destroy(&l_pAApp->protected.Mutex);
    free(l_pAApp);
}
static VOID ANA_rOnStart(PST_A_NATIVE_ACTIVITY in_pNativeActivity) {    
    LOG_V("Start: %p\n", in_pNativeActivity);
    AAPP_rSetActivityState((PST_AAPP)in_pNativeActivity->instance, DE_APPCOM_START);
}
static VOID ANA_rOnResume(PST_A_NATIVE_ACTIVITY in_pNativeActivity) {
    LOG_V("Resume: %p\n", in_pNativeActivity);
    AAPP_rSetActivityState((PST_AAPP)in_pNativeActivity->instance, DE_APPCOM_RESUME);
}
static VOID ANA_rOnPause(PST_A_NATIVE_ACTIVITY in_pNativeActivity) {    
    LOG_V("Pause: %p\n", in_pNativeActivity);
    AAPP_rSetActivityState((PST_AAPP)in_pNativeActivity->instance, DE_APPCOM_PAUSE);
}
static VOID ANA_rOnStop(PST_A_NATIVE_ACTIVITY in_pNativeActivity) {
    LOG_V("Stop: %p\n", in_pNativeActivity);
    AAPP_rSetActivityState((PST_AAPP)in_pNativeActivity->instance, DE_APPCOM_STOP);
}
static VOID ANA_rOnConfigurationChanged(PST_A_NATIVE_ACTIVITY in_pNativeActivity) {
    LOG_V("ConfigurationChanged: %p\n", in_pNativeActivity);
    AAPP_rWriteCom((PST_AAPP)in_pNativeActivity->instance, DE_APPCOM_CONFIG_CHANGED);
}
static VOID ANA_rOnLowMemory(PST_A_NATIVE_ACTIVITY in_pNativeActivity) {
    LOG_V("LowMemory: %p\n", in_pNativeActivity);
    AAPP_rWriteCom((PST_AAPP)in_pNativeActivity->instance, DE_APPCOM_LOW_MEMORY);
}
static VOID ANA_rOnWindowFocusChanged(PST_A_NATIVE_ACTIVITY in_pNativeActivity, BOOL in_bFocused) {
    LOG_V("WindowFocusChanged: %p -- %d\n", in_pNativeActivity, in_bFocused);
    AAPP_rWriteCom((PST_AAPP)in_pNativeActivity->instance, in_bFocused ? DE_APPCOM_FOCUS_GAINED : DE_APPCOM_FOCUS_LOST);
}
static VOID ANA_rOnNativeWindowCreated(PST_A_NATIVE_ACTIVITY in_pNativeActivity, PST_A_NATIVE_WINDOW in_pNativeWindow) {    
    LOG_V("NativeWindowCreated: %p -- %p\n", in_pNativeActivity, in_pNativeWindow);
    AAPP_rSetNativeWindow((PST_AAPP)in_pNativeActivity->instance, in_pNativeWindow);
}
static VOID ANA_rOnNativeWindowDestroyed(PST_A_NATIVE_ACTIVITY in_pNativeActivity, PST_A_NATIVE_WINDOW in_pNativeWindow) {
    LOG_V("NativeWindowDestroyed: %p -- %p\n", in_pNativeActivity, in_pNativeWindow);
    AAPP_rSetNativeWindow((PST_AAPP)in_pNativeActivity->instance, NULL);
}
static VOID ANA_rOnInputQueueCreated(PST_A_NATIVE_ACTIVITY in_pNativeActivity, PST_A_INPUT_QUEUE in_pInputQueue) {
    LOG_V("InputQueueCreated: %p -- %p\n", in_pNativeActivity, in_pInputQueue);
    AAPP_rSetInputQueue((PST_AAPP)in_pNativeActivity->instance, in_pInputQueue);
}
static VOID ANA_rOnInputQueueDestroyed(PST_A_NATIVE_ACTIVITY in_pNativeActivity, PST_A_INPUT_QUEUE in_pInputQueue) {
    LOG_V("InputQueueDestroyed: %p -- %p\n", in_pNativeActivity, in_pInputQueue);
    AAPP_rSetInputQueue((PST_AAPP)in_pNativeActivity->instance, NULL);
}
static PVOID ANA_rOnSaveInstanceState(PST_A_NATIVE_ACTIVITY in_pNativeActivity, PSIZE_T out_nLen) {
    PST_AAPP l_pAApp = (PST_AAPP)in_pNativeActivity->instance;
    PVOID l_pSavedState = NULL;

    LOG_V("SaveInstanceState: %p\n", in_pNativeActivity);
    pthread_mutex_lock(&l_pAApp->protected.Mutex);
    l_pAApp->protected.bSaved = 0;
    AAPP_rWriteCom(l_pAApp, DE_APPCOM_SAVE_STATE);
    // Ждём сигнала от потока приложения, что оно сохранило своё состояние...
    while (!l_pAApp->protected.bSaved) {
        pthread_cond_wait(&l_pAApp->protected.Cond, &l_pAApp->protected.Mutex);
    }
    if (l_pAApp->pSavedState) {
        l_pSavedState = l_pAApp->pSavedState;
        *out_nLen = l_pAApp->nSavedStateSize;
        l_pAApp->pSavedState = NULL;
        l_pAApp->nSavedStateSize = 0;
    }
    pthread_mutex_unlock(&l_pAApp->protected.Mutex);
    return l_pSavedState;
}
static VOID AAPP_rSetActivityState(PST_AAPP in_pAApp, INT in_iCom) {
    pthread_mutex_lock(&in_pAApp->protected.Mutex);
    AAPP_rWriteCom(in_pAApp, in_iCom);
    // Ждём сигнала от потока приложения, что оно сменило состояние активити...
    while (in_pAApp->iActivityState != in_iCom) {
        pthread_cond_wait(&in_pAApp->protected.Cond, &in_pAApp->protected.Mutex);
    }
    pthread_mutex_unlock(&in_pAApp->protected.Mutex);
}
static VOID AAPP_rSetInputQueue(PST_AAPP in_pAApp, PST_A_INPUT_QUEUE in_pInputQueue) {
    pthread_mutex_lock(&in_pAApp->protected.Mutex);
    in_pAApp->protected.pInputQueue = in_pInputQueue;
    AAPP_rWriteCom(in_pAApp, DE_APPCOM_INPUT_CHANGED);
    // Ждём сигнала от потока приложения, что оно сменило цепочку событий инпута...
    while (in_pAApp->pInputQueue != in_pAApp->protected.pInputQueue) {
        pthread_cond_wait(&in_pAApp->protected.Cond, &in_pAApp->protected.Mutex);
    }
    pthread_mutex_unlock(&in_pAApp->protected.Mutex);
}
static VOID AAPP_rSetNativeWindow(PST_AAPP in_pAApp, PST_A_NATIVE_WINDOW in_pNativeWindow){
    pthread_mutex_lock(&in_pAApp->protected.Mutex);
    // Если есть старое окно, разрушаем его...
    if (in_pAApp->protected.pNativeWindow) {
        AAPP_rWriteCom(in_pAApp, DE_APPCOM_WINDOW_TERM);
    }
    if((in_pAApp->protected.pNativeWindow = in_pNativeWindow)) {
        AAPP_rWriteCom(in_pAApp, DE_APPCOM_WINDOW_INIT);
    }
    // Ждём сигнала от потока приложения, что оно сменило окно...
    while (in_pAApp->pNativeWindow != in_pAApp->protected.pNativeWindow) {
        pthread_cond_wait(&in_pAApp->protected.Cond, &in_pAApp->protected.Mutex);
    }
    pthread_mutex_unlock(&in_pAApp->protected.Mutex);
}
static VOID AAPP_rWriteCom(PST_AAPP in_pAApp, BYTE in_iCom) {
    if (write(in_pAApp->protected.iMsgWrite, &in_iCom, sizeof(in_iCom)) != sizeof(in_iCom)) {
        LOG_E("Failure writing to android app command: %s\n", strerror(errno));
    }
}
static VOID AAPP_rFreeSavedState(PST_AAPP in_pAApp) {    
    pthread_mutex_lock(&in_pAApp->protected.Mutex);
    if (in_pAApp->pSavedState) {
        free(in_pAApp->pSavedState);
        in_pAApp->pSavedState = NULL;
        in_pAApp->nSavedStateSize = 0;
    }
    pthread_mutex_unlock(&in_pAApp->protected.Mutex);
}
static VOID LOG_rPrintConfiguration(PST_A_CONFIGURATION in_pConfiguration) {
    CHAR lang[2], country[2];
    AConfiguration_getLanguage(in_pConfiguration, lang);
    AConfiguration_getCountry(in_pConfiguration, country);

    LOG_V("Config: mcc=%d mnc=%d lang=%c%c cnt=%c%c orien=%d touch=%d dens=%d "
            "keys=%d nav=%d keysHid=%d navHid=%d sdk=%d size=%d long=%d "
            "modetype=%d modenight=%d",
            AConfiguration_getMcc(in_pConfiguration),
            AConfiguration_getMnc(in_pConfiguration),
            lang[0], lang[1], country[0], country[1],
            AConfiguration_getOrientation(in_pConfiguration),
            AConfiguration_getTouchscreen(in_pConfiguration),
            AConfiguration_getDensity(in_pConfiguration),
            AConfiguration_getKeyboard(in_pConfiguration),
            AConfiguration_getNavigation(in_pConfiguration),
            AConfiguration_getKeysHidden(in_pConfiguration),
            AConfiguration_getNavHidden(in_pConfiguration),
            AConfiguration_getSdkVersion(in_pConfiguration),
            AConfiguration_getScreenSize(in_pConfiguration),
            AConfiguration_getScreenLong(in_pConfiguration),
            AConfiguration_getUiModeType(in_pConfiguration),
            AConfiguration_getUiModeNight(in_pConfiguration));
}

static PVOID MAIN_rAppEntry(PVOID in_pData) {
    PST_AAPP l_pAApp = (PST_AAPP)in_pData;
    l_pAApp->pConfiguration = AConfiguration_new();
    AConfiguration_fromAssetManager(l_pAApp->pConfiguration, l_pAApp->pNativeActivity->assetManager);
    LOG_rPrintConfiguration(l_pAApp->pConfiguration);

    // l_pAApp->protected.apsrcCom.nId = DE_LOOPER_ID_MAIN;
    // l_pAApp->protected.apsrcCom.pAApp = l_pAApp;
    // l_pAApp->protected.apsrcCom.prProc = process_cmd;
    // l_pAApp->protected.apsrcInput.nId = DE_LOOPER_ID_INPUT;
    // l_pAApp->protected.apsrcInput.pAApp = l_pAApp;
    // l_pAApp->protected.apsrcInput.prProc = process_input;

    PST_A_LOOPER l_pLooper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    ALooper_addFd(l_pLooper,  l_pAApp->protected.iMsgRead, DE_LOOPER_ID_MAIN, ALOOPER_EVENT_INPUT, NULL, NULL/*&l_pAApp->protected.apsrcCom*/);
    l_pAApp->pLooper = l_pLooper;

    pthread_mutex_lock(&l_pAApp->protected.Mutex);
    l_pAApp->protected.bRunning = 1;
    pthread_cond_broadcast(&l_pAApp->protected.Cond);
    pthread_mutex_unlock(&l_pAApp->protected.Mutex);



    INT l_iIdent;
    INT l_iEvents;
    PVOID l_pData;
    while(1) {
        while((l_iIdent = ALooper_pollAll((l_pAApp->bAnimating ? 0 : -1), NULL, &l_iEvents, (PVOID*)&l_pData)) >= 0) {
            switch (l_iIdent) {
                case DE_LOOPER_ID_MAIN:
                    MAIN_rProcCom(l_pAApp);
                    break;
                case DE_LOOPER_ID_INPUT:
                    MAIN_rProcInput(l_pAApp);
                    break;
                case DE_LOOPER_ID_USER:
                    break;
                default:
                    break;
            };
            if (l_pAApp->iDestroyRequested) {
                // ENGINE_rTermDisplay(&m_ENGINE);
                goto goto_pointer_END;
            }
        }
        if (l_pAApp->bAnimating) {
            // ENGINE_rDrawFrame(&m_ENGINE);
        }
    }
    goto_pointer_END: ;

    LOG_V("android app destroy!");
    AAPP_rFreeSavedState(l_pAApp);
    pthread_mutex_lock(&l_pAApp->protected.Mutex);
    if (l_pAApp->pInputQueue) {
        AInputQueue_detachLooper(l_pAApp->pInputQueue);
    }
    AConfiguration_delete(l_pAApp->pConfiguration);
    l_pAApp->protected.bDestroyed = 1;
    pthread_cond_broadcast(&l_pAApp->protected.Cond);
    pthread_mutex_unlock(&l_pAApp->protected.Mutex);

    return NULL;
}
static VOID MAIN_rProcInput(PST_AAPP in_pAApp) {
    PST_A_INPUT_EVENT l_pInputEvent = NULL;
    while (AInputQueue_getEvent(in_pAApp->pInputQueue, &l_pInputEvent) >= 0) {
        LOG_V("New input event: type=%d\n", AInputEvent_getType(l_pInputEvent));
        if (AInputQueue_preDispatchEvent(in_pAApp->pInputQueue, l_pInputEvent)) {
            continue;
        }
        INT32 l_iHandled = 0;
        if (in_pAApp->prOnInputEvent) l_iHandled = in_pAApp->prOnInputEvent(in_pAApp, l_pInputEvent);
        AInputQueue_finishEvent(in_pAApp->pInputQueue, l_pInputEvent, l_iHandled);
    }
}
static VOID MAIN_rProcCom(PST_AAPP in_pAApp) {
    BYTE l_i8Com = 0;
    INT l_iCom = -1;
    if (read(in_pAApp->protected.iMsgRead, &l_i8Com, sizeof(l_i8Com)) == sizeof(l_i8Com)) {
        l_iCom = l_i8Com;
    } else {
        LOG_E("No data on command pipe!");
        return;
    }
    switch(l_iCom) {
        case DE_APPCOM_INPUT_CHANGED:
            LOG_V("DE_APPCOM_INPUT_CHANGED\n");
            pthread_mutex_lock(&in_pAApp->protected.Mutex);
            if (in_pAApp->pInputQueue) {
                AInputQueue_detachLooper(in_pAApp->pInputQueue);
            }
            in_pAApp->pInputQueue = in_pAApp->protected.pInputQueue;
            if (in_pAApp->pInputQueue) {
                LOG_V("Attaching input queue to looper");
                AInputQueue_attachLooper(in_pAApp->pInputQueue,
                        in_pAApp->pLooper, DE_LOOPER_ID_INPUT, NULL,
                        NULL/*&in_pAApp->protected.apsrcInput*/);
            }
            pthread_cond_broadcast(&in_pAApp->protected.Cond);
            pthread_mutex_unlock(&in_pAApp->protected.Mutex);
            if (in_pAApp->prAppCom) in_pAApp->prAppCom(in_pAApp, l_iCom);
            break;
        case DE_APPCOM_WINDOW_INIT:        
            LOG_V("APP_CMD_INIT_WINDOW\n");
            pthread_mutex_lock(&in_pAApp->protected.Mutex);
            in_pAApp->pNativeWindow = in_pAApp->protected.pNativeWindow;
            pthread_cond_broadcast(&in_pAApp->protected.Cond);
            pthread_mutex_unlock(&in_pAApp->protected.Mutex);
            if (in_pAApp->prAppCom) in_pAApp->prAppCom(in_pAApp, l_iCom);
            break;
        case DE_APPCOM_WINDOW_TERM:            
            LOG_V("APP_CMD_TERM_WINDOW\n");
            pthread_cond_broadcast(&in_pAApp->protected.Cond);
            if (in_pAApp->prAppCom) in_pAApp->prAppCom(in_pAApp, l_iCom);            
            LOG_V("APP_CMD_TERM_WINDOW\n");
            pthread_mutex_lock(&in_pAApp->protected.Mutex);
            in_pAApp->pNativeWindow = NULL;
            pthread_cond_broadcast(&in_pAApp->protected.Cond);
            pthread_mutex_unlock(&in_pAApp->protected.Mutex);
            break;
        case DE_APPCOM_START:
        case DE_APPCOM_RESUME:
        case DE_APPCOM_PAUSE:
        case DE_APPCOM_STOP:        
            LOG_V("activityState=%d\n", l_iCom);
            pthread_mutex_lock(&in_pAApp->protected.Mutex);
            in_pAApp->iActivityState = l_iCom;
            pthread_cond_broadcast(&in_pAApp->protected.Cond);
            pthread_mutex_unlock(&in_pAApp->protected.Mutex);
            if (in_pAApp->prAppCom) in_pAApp->prAppCom(in_pAApp, l_iCom); 
            if (l_iCom == DE_APPCOM_RESUME) {
                AAPP_rFreeSavedState(in_pAApp);
            }
            break;
        case DE_APPCOM_SAVE_STATE:        
            AAPP_rFreeSavedState(in_pAApp);
            if (in_pAApp->prAppCom) in_pAApp->prAppCom(in_pAApp, l_iCom);
            LOG_V("APP_CMD_SAVE_STATE\n");
            pthread_mutex_lock(&in_pAApp->protected.Mutex);
            in_pAApp->protected.bSaved = 1;
            pthread_cond_broadcast(&in_pAApp->protected.Cond);
            pthread_mutex_unlock(&in_pAApp->protected.Mutex);
            break;
        case DE_APPCOM_CONFIG_CHANGED:        
            LOG_V("APP_CMD_CONFIG_CHANGED\n");
            AConfiguration_fromAssetManager(in_pAApp->pConfiguration, in_pAApp->pNativeActivity->assetManager);
            LOG_rPrintConfiguration(in_pAApp->pConfiguration);
            if (in_pAApp->prAppCom) in_pAApp->prAppCom(in_pAApp, l_iCom);
            break;
        case DE_APPCOM_DESTROY:
            LOG_V("APP_CMD_DESTROY\n");
            in_pAApp->iDestroyRequested = 1;
            if (in_pAApp->prAppCom) in_pAApp->prAppCom(in_pAApp, l_iCom);
            break;
        default:
            if (in_pAApp->prAppCom) in_pAApp->prAppCom(in_pAApp, l_iCom);   
            break;
    };
}
