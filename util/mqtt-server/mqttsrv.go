// -*- tab-width: 2; fill-column: 100; indent-tabs-mode: t -*-

// Original: https://github.com/eclipse/paho.mqtt.golang/blob/master/cmd/stdoutsub/main.go
// Modified from the original.

/*
 * Copyright (c) 2021 IBM Corp and others.
 *
 * All rights reserved. This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v2.0 and Eclipse Distribution License v1.0 which accompany
 * this distribution.
 *
 * The Eclipse Public License is available at
 *    https://www.eclipse.org/legal/epl-2.0/
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Seth Hoenig
 *    Allan Stockdill-Mander
 *    Mike Robertson
 */

package main

import (
	"crypto/tls"
	"fmt"
	"log"
	"math/rand"
	"os"
	"os/signal"
	"strings"
	"syscall"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

const (
	server = "tcp://127.0.0.1:1883"
	client_id = "mqttsrv"
	username = "guest"
	password = "guest"
)

func onStartupReceived(client mqtt.Client, message mqtt.Message) {
	logMessage("STARTUP.log", message)
	t := message.Topic()
	// "snappy/startup/class/device"
	fields := strings.Split(t, "/")
	client.Publish(
		"snappy/control/" + fields[3],
		/* qos= */ 1,
		/* retained= */ false,
		fmt.Sprintf("{\"interval\": %d}", (5+rand.Intn(56))*60))
}

func onObservationReceived(client mqtt.Client, message mqtt.Message) {
	logMessage("OBSERVATION.log", message)
}

func logMessage(filename string, message mqtt.Message) {
	f, err := os.OpenFile(filename, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		log.Print(err)
		return
	}
	defer f.Close()
	obs := fmt.Sprintf("{\"topic\":\"%s\", \"payload\":%s}\n", message.Topic(), message.Payload())
	if _, err := f.Write([]byte(obs)); err != nil {
		return
	}
}

func main() {
	// mqtt.DEBUG = log.New(os.Stdout, "", 0)
	// mqtt.ERROR = log.New(os.Stdout, "", 0)
	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt, syscall.SIGTERM)

	connOpts := mqtt.NewClientOptions().AddBroker(server).SetClientID(client_id).SetCleanSession(true)
	connOpts.SetUsername(username)
	connOpts.SetPassword(password)

	tlsConfig := &tls.Config{InsecureSkipVerify: true, ClientAuth: tls.NoClientCert}
	connOpts.SetTLSConfig(tlsConfig)

	connOpts.OnConnect = func(c mqtt.Client) {
		if token := c.Subscribe("snappy/startup/#", /* qos= */ 1, onStartupReceived); token.Wait() && token.Error() != nil {
			panic(token.Error())
		}
		if token := c.Subscribe("snappy/observation/#", /* qos= */ 1, onObservationReceived); token.Wait() && token.Error() != nil {
			panic(token.Error())
		}
	}

	client := mqtt.NewClient(connOpts)
	if token := client.Connect(); token.Wait() && token.Error() != nil {
		panic(token.Error())
	} else {
		fmt.Printf("Connected to %s\n", server)
	}

	<-c
}
