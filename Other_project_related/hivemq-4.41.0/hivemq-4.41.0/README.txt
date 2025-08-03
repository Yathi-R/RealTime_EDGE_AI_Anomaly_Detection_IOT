HiveMQ Platform
===============
The HiveMQ Platform is prepackaged with all HiveMQ Enterprise Extensions (disabled),
the open-source MQTT CLI tool, and the HiveMQ Swarm load-testing tool (both located
in the tools folder of your HiveMQ installation).

HiveMQ
======
To learn more about the features and configuration options the HiveMQ Platform offers,
see the HiveMQ User Guide:
https://docs.hivemq.com/hivemq/latest/user-guide/getting-started.html#download

HiveMQ Quick Start
==================

Linux/Unix/macOS
----------------
cd <hivemq-directory>/bin
./run.sh

Windows
-------
right click <hivemq-directory>\bin\run.bat
select "Run as administrator"


HiveMQ Enterprise Extensions
============================
For a list of the HiveMQ Enterprise Extensions the HiveMQ Platform includes and detailed information
on how to enable and configure each extension, see the HiveMQ Enterprise Extension documentation:
https://www.hivemq.com/docs/hivemq/latest/enterprise-extensions/introduction.html

HiveMQ Enterprise Extensions Quick Start
========================================

Linux/Unix/macOS
----------------
cd <hivemq-directory>/extensions/<hivemq-enterprise-extension-name>
rm -rf DISABLED

Windows
-------
right click <hivemq-directory>\extensions\<hivemq-enterprise-extension-name>\DISABLED
select "delete"


MQTT CLI
========
The MQTT CLI is an open-source Java MQTT client tool that enables you to interact with an MQTT broker
in various ways. For detailed information on all supported commands, see the MQTT CLI documentation:
https://hivemq.github.io/mqtt-cli/docs/quick-start/

Linux/Unix/macOS
----------------
cd <hivemq-directory>/tools/mqtt-cli/bin
./mqtt-cli

Windows
-------
open the command prompt
cd <hivemq-directory>\tools\mqtt-cli\bin
mqtt-cli.bat


HiveMQ Swarm
============
To set up complete end-to-end testing of your IoT solution with the HiveMQ Swarm distributed
simulation environment tool, see the HiveMQ Swarm documentation:
https://docs.hivemq.com/swarm/latest/swarm/swarm-intro.html


HiveMQ Kubernetes Operator
==========================
To manage your HiveMQ deployment with the HiveMQ Kubernetes Operator,
see the HiveMQ Kubernetes Operator documentation:
https://docs.hivemq.com/operator/latest/kubernetes-operator/operator-intro.html#quick-start

