############### santec XNS Data Management Sample Code ######################
############### October 2023 #########################################
# install the required library first on the command line by running:
# python -m pip install requests
# make sure the "server.py" file is in the same directory as apiSample.py

# this script can be run from the command line via "python apiSample.py"
# assuming Python has been added to your PATH

# required imports (server is the 'server.py' file)
import server
import requests
import socket

# function to setup a subscriber at the url given in "data".
# this subscriber will be notified when new data is sent to the XNS
def set_subscriber(ip,data, timeout=100):
    #send the request to the API
    webResp=requests.post( "http://" +ip +":8083/api/subscribers/create-subscriber", json=data)

    #check if the response was "200- OK", otherwise just return None
    if (webResp.status_code == requests.codes.ok):
        webRespJson = webResp.json()
        return webRespJson
    else:
        return None

# function to get measurement data from the given serial number
def get_results(ip,serial):
    #send the request to the API
    webResp=requests.get("http://" +ip +":8083/integration/results/combined-measurements?dutSn="+serial)

     #check if the response was "200- OK", otherwise just return None
    if (webResp.status_code == requests.codes.ok):
        webRespJson = webResp.json()
        return webRespJson
    else:
        return None

# function to automatically retrieve the IP address of your PC
# this will be used to subscribe to XNS results so that results can be displayed in your terminal
def get_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(0)
    try:
        # doesn't even have to be reachable
        s.connect(('10.254.254.254', 1))
        IP = s.getsockname()[0]
    except Exception:
        IP = '127.0.0.1'
    finally:
        s.close()

    return IP


##################### main script ############################
#ask for XNS IP address
serverIP=input('What is the XNS IP address? (if running on this PC, type "localhost")  ')


# the notification will be sent to a simple http server that we are running locally for this demo
# it is entirely possible to run the XNS and the notification server on the local PC without ever using 
# an actual external network connection
myIP=get_ip()

# this is what subscriber data should look like. 
subscriberdata = {
    "url": "http://"+myIP+":8555"
}
# create a subscriber on the XNS with the http server we just set up
print('')
print(set_subscriber(serverIP,subscriberdata))

# get a result for a specific serial number (edit "sn" with an actual serial number contained in the XNS database)
# you should see a JSON object printed out here
# this object represents the measurement data of the last test run for that SN
sn='sn'
print('')
print('Result for serial number: '+str(sn))
print(get_results(serverIP,sn))
print('')

# setup python http server to see how subscribers work
server.run()

# now try running an actual test in the Cable Assembly Application 
# notifications should appear in your terminal in real time











