/*
 * ir-gateway, build to automate ir remote control commands in smart homes.
 *
 * Copyright (C) 2025 Julian Friedrich
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>. 
 *
 * You can file issues at https://github.com/fjulian79/ir-gateway/issues
 */

 #pragma once

#include <WebServer.h>
#include <Arduino.h>

/**
 * @brief Web server control class.
 * This class handles the web server functionality for the ir-gateway.
 */
class WebServerControl {

    public:

        /**
         * @brief Constructor for WebServerControl.
         * @param port The port to run the web server on, default is 80.
         */
        WebServerControl(int port = 80);
        
        /**
         * @brief Destructor for WebServerControl.
         * Cleans up resources used by the web server.
         */
        ~WebServerControl();
        
        /**
         * @brief Start the web server.
         * This method initializes and starts the web server on the specified port.
         */
        void begin();
        
        /**
         * @brief Stop the web server.
         * This method stops the web server and releases any resources used.
         */
        void stop();
        
        /**
         * @brief Handle incoming client requests.
         * This method processes client requests and routes them to the appropriate handlers.
         */
        void handleClient();
        
        /**
         * @brief Check if the web server is running.
         * @return true if the web server is running, false otherwise.
         */
        bool isEnabled() const;
        
        /**
         * @brief Get the port the web server is running on.
         * @return The port number.
         */
        int getPort() const;

    private:

        /**
         * @brief Setup the routes for the web server.
         * This method defines the routes and their corresponding handlers.
         */
        void setupRoutes();
        
        /**
         * @brief Handle the root path.
         * This method processes requests to the root path ("/").
         */
        void handleRoot();
        
        /**
         * @brief Handle the transmission of IR signals.
         * This method processes requests to transmit IR signals.
         */
        void handleTx();
        
        /**
         * @brief Handle sequence transmission requests.
         */
        void handleTxSequence();
        
        /**
         * @brief Execute a sequence of IR commands.
         * @param sequence The sequence string to execute.
         * @param errorMessage Reference to store error messages.
         * @return Number of executed commands, or -1 on error.
         */
        int executeSequence(const String& sequence, String& errorMessage);
        
        /**
         * @brief Execute a single command from a sequence.
         * @param command The command string to execute.
         * @param errorMessage Reference to store error messages.
         * @return True on success, false on error.
         */
        bool executeSequenceCommand(const String& command, String& errorMessage);

        /**
         * @brief Handle the reception of IR signals.
         * This method processes requests to receive IR signals.
         */
        void handleTxLog();
        
        /**
         * @brief Handle the reception log of IR signals.
         * This method processes requests to view the reception log.
         */
        void handleRxLog();
        
        /**
         * @brief Handle the configuration of the web server.
         * This method processes requests to configure the web server settings.
         */
        void handleNotFound();
        
        /**
         * @brief Web server instance.
         * This object represents the web server.
         */
        WebServer Server;
        
        /**
         * @brief The port the web server is running on.
         */
        int Port;
        
        /**
         * @brief Flag indicating if the web server is running.
         * This flag is set to true when the server is started and false when it is stopped.
         */
        bool Enabled;

};
