/*  Licensed to the Apache Software Foundation (ASF) under one
    or more contributor license agreements.  See the NOTICE file
    distributed with this work for additional information
    regarding copyright ownership.  The ASF licenses this file
    to you under the Apache License, Version 2.0 (the
    "License"); you may not use this file except in compliance
    with the License.  You may obtain a copy of the License at
 
    http://www.apache.org/licenses/LICENSE-2.0
 
    Unless required by applicable law or agreed to in writing,
    software distributed under the License is distributed on an
    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
    KIND, either express or implied.  See the License for the
    specific language governing permissions and limitations
    under the License.
 */

#include <pebble.h>
#include "includes/RideTracker.h"

enum {
    KEY_DISTANCE,
    KEY_OTHER,
};

Window *window;

TextLayer *timeLabel;
TextLayer *timeText;

TextLayer *distanceLabel;
TextLayer *distanceText;

TextLayer *startStopButton;
TextLayer *resetButton;

Layer *rootLayer;

GFont *gothic18;
GFont *roboto21;

int elapsedTime = 0;
int hours = 0;
int minutes = 0;

float distanceTravelled = 0.0f;

bool started = false;

int main(void) {
    init();
    app_event_loop();
    deinit();
    
    return 0;
}

void init() {    
    app_message_register_inbox_received(in_received_handler);
    app_message_register_inbox_dropped(in_dropped_handler);
    app_message_register_outbox_sent(out_sent_handler);
    app_message_register_outbox_failed(out_failed_handler);
    
    const uint32_t inbound_size = 64;
    const uint32_t outbound_size = 64;
    app_message_open(inbound_size, outbound_size);

    window = window_create();
    rootLayer = window_get_root_layer(window);
    window_stack_push(window, true /* Animated */);
    window_set_background_color(window, GColorBlack);
    window_set_click_config_provider(window, click_config_provider);
    
    gothic18 = fonts_get_system_font(FONT_KEY_GOTHIC_18);
    roboto21 = fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21);
    
    //Time label setup
    timeLabel = text_layer_create(GRect(5, 10, 60, 25));
    text_layer_set_text(timeLabel, "Time");
    text_layer_set_font(timeLabel, gothic18);
    text_layer_set_background_color(timeLabel, GColorBlack);
    text_layer_set_text_color(timeLabel, GColorWhite);
    
    //Time text setup
    timeText = text_layer_create(GRect(5, 35, 60, 30));
    text_layer_set_background_color(timeText, GColorWhite);
    text_layer_set_font(timeText, roboto21);
    text_layer_set_text(timeText, "0:00");
    text_layer_set_text_color(timeText, GColorBlack);
    text_layer_set_text_alignment(timeText, GTextAlignmentRight);
    
    //Distance label setup
    distanceLabel = text_layer_create(GRect(5, 85, 85, 25));
    text_layer_set_text(distanceLabel, "Distance (km)");
    text_layer_set_font(distanceLabel, gothic18);
    text_layer_set_background_color(distanceLabel, GColorBlack);
    text_layer_set_text_color(distanceLabel, GColorWhite);
    
    //Distance text setup
    distanceText = text_layer_create(GRect(5, 110, 60, 30));
    text_layer_set_background_color(distanceText, GColorWhite);
    text_layer_set_font(distanceText, roboto21);
    text_layer_set_text(distanceText, "0.00");
    text_layer_set_text_color(distanceText, GColorBlack);
    text_layer_set_text_alignment(distanceText, GTextAlignmentRight);
    
    //Start/Stop Button setup
    startStopButton = text_layer_create(GRect(109, 10, 30, 25));
    text_layer_set_text(startStopButton, "start");
    text_layer_set_font(startStopButton, gothic18);
    text_layer_set_background_color(startStopButton, GColorBlack);
    text_layer_set_text_color(startStopButton, GColorWhite);
    text_layer_set_text_alignment(startStopButton, GTextAlignmentRight);
    
    //Reset Button setup
    resetButton = text_layer_create(GRect(109, 110, 30, 25));
    text_layer_set_text(resetButton, "reset");
    text_layer_set_font(resetButton, gothic18);
    text_layer_set_background_color(resetButton, GColorBlack);
    text_layer_set_text_color(resetButton, GColorWhite);
    text_layer_set_text_alignment(resetButton, GTextAlignmentRight);
    
    //Add the textlayers to the window
    layer_add_child(rootLayer, text_layer_get_layer(timeLabel));
    layer_add_child(rootLayer, text_layer_get_layer(timeText));
    layer_add_child(rootLayer, text_layer_get_layer(distanceLabel));
    layer_add_child(rootLayer, text_layer_get_layer(distanceText));
    layer_add_child(rootLayer, text_layer_get_layer(startStopButton));
    layer_add_child(rootLayer, text_layer_get_layer(resetButton));

    tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
}

void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
    if (started) {
        elapsedTime++;
        
        if (elapsedTime%60 == 0) {
            minutes = 0;
            hours++;
        } else {
            minutes++;
        }
        
        updateTime(hours, minutes);
    }
}

void click_config_provider(void *context) {
    //Register Start/Stop Button click handler
    window_single_click_subscribe(BUTTON_ID_UP, handle_startStop_click);
    
    //Register Reset Buttonclick handler
    window_single_click_subscribe(BUTTON_ID_DOWN, handle_reset_click);
}

void handle_startStop_click(ClickRecognizerRef recognizer, void *context) {
    started = !started;
    if (started) {
        text_layer_set_text(startStopButton, "stop");
        layer_set_hidden(text_layer_get_layer(resetButton), true);
    } else {
        text_layer_set_text(startStopButton, "start");
        layer_set_hidden(text_layer_get_layer(resetButton), false);
    }
    sendStartCommand(started);
}

void handle_reset_click(ClickRecognizerRef recognizer, void *context) {
    if (!started) {
        elapsedTime = 0;
        hours = 0;
        minutes = 0;
        distanceTravelled = 0.0f;
        updateTime(hours, minutes);
        //TODO: Broadcast to the phone to reset there as well
    }
}

void updateTime(int hours, int minutes) {
    char* timeString = "00:00 ";
    snprintf(timeString, 6, "%d:%02d", hours, minutes);
    text_layer_set_text(timeText, timeString);
}

void updateDistance(int metres) {
    float kilometres = metres/1000;
    char* kmString = "999 ";
    snprintf(kmString, 4, "%.1f", kilometres);
    text_layer_set_text(distanceText, kmString);
}

void in_received_handler(DictionaryIterator *iter, void *context) {
    Tuple *distanceTuple = dict_find(iter, KEY_DISTANCE);
    
    if (distanceTuple) {
        updateDistance(distanceTuple->value->uint32);
    }
}

void in_dropped_handler(AppMessageResult reason, void *context) {
    //TODO: At least log the dropped message
}

void out_sent_handler(DictionaryIterator *sent, void *conttext) {
    // Nothing to be done here for now
}

void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
    //TODO: Log the failed send, see if we want to save & re-send it
}

void deinit(void) {
    text_layer_destroy(resetButton);
    text_layer_destroy(startStopButton);
    text_layer_destroy(distanceText);
    text_layer_destroy(distanceLabel);
    text_layer_destroy(timeText);
    text_layer_destroy(timeLabel);
    
    window_destroy(window);
    
    tick_timer_service_unsubscribe();
}

void sendStartCommand(bool start) {
    if (bluetooth_connection_service_peek()) {
        DictionaryIterator *iter;
        app_message_outbox_begin(&iter);
        if (start) {
            Tuplet value = TupletCString(0, "true");
            dict_write_tuplet(iter, &value);
        } else {
            Tuplet value = TupletCString(0, "false");
            dict_write_tuplet(iter, &value);
        }
        app_message_outbox_send();
    }
}
