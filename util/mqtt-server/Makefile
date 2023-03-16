.PHONY: start container run volume seed

RABBITMQ_VERSION=3.11
MQTT_CONTAINER=rabbitmq-mqtt:$(RABBITMQ_VERSION)
MQTT_NAME=snappy-mqtt
MQTT_VOLUME=snappy-mqtt-data

start:
	@echo "Choose an explicit target"

# Create a container with 
container:
	@echo "# syntax=docker/dockerfile:1" > rabbitmq-dockerfile
	@echo "FROM rabbitmq:$(RABBITMQ_VERSION)" >> rabbitmq-dockerfile
	@echo "RUN rabbitmq-plugins enable --offline rabbitmq_mqtt" >> rabbitmq-dockerfile
	docker build -f rabbitmq-dockerfile -t $(MQTT_CONTAINER) .
	@rm rabbitmq-dockerfile

# Start the container
run:
	docker run -d --name $(MQTT_NAME) -p 1883:1883 -v $(MQTT_VOLUME):/var/lib/rabbitmq $(MQTT_CONTAINER)

# Create a new volume for the MQTT container, do this before running the container the first time
volume:
	docker volume create $(MQTT_VOLUME)

# Seed a new volume with user information, you only need to do this once after creating it,
# but the container must be running
seed:
	docker exec $(MQTT_NAME) rabbitmqctl add_user lth qumquat
	docker exec $(MQTT_NAME) rabbitmqctl set_permissions lth '.*' '.*' '.*'
