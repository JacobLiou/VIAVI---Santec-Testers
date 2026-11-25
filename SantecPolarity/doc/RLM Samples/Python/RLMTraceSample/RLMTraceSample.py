# make sure NIVISA or R&S VISA is installed on the system: https://www.rohde-schwarz.com/us/applications/r-s-visa-application-note_56280-148812.html
# install the requests library via "py -m pip install requests"
# install the pyvisa library via "py -m pip install pyvisa"
# install the matplot library via "py -m pip install matplotlib"
import pyvisa as vs
import requests
import time
import matplotlib.pyplot as plt

####### constants #####################################################################
# USBTMC device connection variables
SANTEC_VID = 'USB0::0x2428'  # 0x2428 is the santec vendor ID (VID)
OSX_100=  'D003'# this is the santec product ID, can be used to find specific types of devices
RLM_100= 'D007'
PLM_100= 'D00A'
PTM_100= 'D008'
OVA_100= 'D006'
OVB_100= 'D00B'
OPM_200= 'D00C'
BRM_100= 'D00A'
TIMEOUT_MS = 15000

####### functions #####################################################################

# retrieve the OTDR trace from an RLM.
# Note: the trace must already exist on the device.
# this can be accomplished by first running a "READ:RL? <wl>" SCPI command
# Note: the trace can ONLY be retrieved over Ethernet at this time
def get_otdr_trace_full(ip, timeout=100):
    webResp = requests.get(
        "http://{}/api/otdr/getOtdrTraceInfo".format(ip), timeout=timeout)

    if (webResp.status_code == requests.codes.ok):
        webRespJson = webResp.json()
        return webRespJson
    else:
        return None

def calculateAndPlot(traceInfo):
    # calculate a few important pieces of information
    deltaD=float(traceInfo["debugInfo"]["deltaD"])/2 # spacing between trace points
    posZero=float(traceInfo["debugInfo"]["zeroPositionIndex"])*deltaD # output of RLM
    eof=int(traceInfo["debugInfo"]['endofFiberTop']) # detected end of fiber
    rlaRef=int(traceInfo["debugInfo"]['rlARefPoint']) # get the stored RLA point
    
    ########### required info to display a nice plot in meters (m) #################
    acqSample=int(traceInfo["debugInfo"]['acqSample'])
    numPhases=int(traceInfo["debugInfo"]['noPhase'])

    offSample=2
    numSample=offSample+acqSample

    x = range(0, 2**numPhases * numSample)
    x=[i*deltaD for i in x]  
    ########### end required plot info ##############################################

    # get the OTDR line data
    traceData = traceInfo["traceData"]
    plt.plot(x, traceData)
    
    plt.title("Trace Data")
    plt.xlabel("Distance (m)")
    plt.ylabel("5Log (dB)")
    plt.annotate("Zero",(posZero,20))
    plt.annotate("EOF",(eof*deltaD,20))
    plt.annotate("rlARefPoint",(rlaRef*deltaD,20))

    plt.show()
    
# automatically discover connected santec USB devices, if none, prompt for IP address.
# Note: santec Ethernet devices can be automatically found by using the MDNS protocol if desired 
def connectInstrument():
    instrument=None
    rm = vs.ResourceManager()
    instruments = rm.list_resources()
    # See if any santec USB instruments are present
    if (any(s.startswith(SANTEC_VID) for s in instruments)):
        r = [s for s in instruments if s.startswith(SANTEC_VID) and RLM_100 in s ]
        #to search for any type of santec instrument, use the line below instead
        #r = [s for s in instruments if s.startswith(SANTEC_VID)]
        if (len(r) > 0):
            #connect to the first one, r[0]
            instrument = rm.open_resource(r[0], open_timeout=TIMEOUT_MS)        
    else:
        #no USB devices, try asking for the IP address
        print("No USB RLM found")
        ip=input("Enter IP Address: ")
        TCP_INST="TCPIP::"+ip+"::5025::SOCKET"
        instrument = rm.open_resource(TCP_INST, open_timeout=5000)
    
    if instrument !=None:
        # read/write terminations are required for Santec products
        instrument.write_termination = '\n'
        instrument.read_termination = '\n'
        instrument.timeout=TIMEOUT_MS
        time.sleep(0.5)

    return instrument

############ main ######################################################################

rlm=connectInstrument()

# get the IP from the instrument, this is required to query the trace later
ip=rlm.query(":SYSTem:COMMunicate:LAN:ADDRess?").replace('"','')
if ip=="no ip":
    print("RLM must also be connected via Ethernet to retrieve trace")
    exit()

# get list of supported wavelengths
wls=(rlm.query("LASER:INFO?")).split(',')

# RLM hold two traces in its buffer. For this demo we only want the most recent trace so throw away the first one.
# clear all previous traces from buffer ONCE after connecting to the RLM
traceInfo = get_otdr_trace_full(ip)
while not (traceInfo==None): #not traceInfo.contains("No traces available")
    traceInfo = get_otdr_trace_full(ip)
    
# run an RL measurement and produce an OTDR trace for the first wavelength
rlm.query("READ:RL? "+wls[0])
traceInfo = get_otdr_trace_full(ip)

# show the trace via matplot
calculateAndPlot(traceInfo)


