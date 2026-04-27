import { useState, useRef, useEffect } from 'react';

function App() {

  // set global resources
  const webSocketIp = "ws://127.0.0.1:5000";
  
  // set useState's
  const [serverResponse, setServerResponse] = useState('');
  const [remoteStreams, setRemoteStreams] = useState([]);
  const [wsState, setWsState] = useState("Closed");
  const [pcState, setPcState] = useState("Closed");
  // set useRef's
  const socketRef = useRef(null);
  const pcRef = useRef(null);
  const peerConnectionRef = useRef("Closed");
  const webSocketRef = useRef("Closed");

  // === Setter Functions ===

  const setWs = (val) => {
    setWsState(val);
    webSocketRef.current = val;
  }

  const setPc = (val) => {
    setPcState(val);
    peerConnectionRef.current = val;
  }


  // === WEBSOCKET CALLBACKS === 
  const onOpen = (event) => {
    console.log("Opening WebSocket - Sending Greeting")

    const msg = {text: "Greetings from react client!"};

    socketRef.current.send(JSON.stringify(msg));

  }

  const onMessage = (event) => {
    console.log("Message Received: ", event.data)
    try{
      const jsonObject = JSON.parse(event.data);
      handleMessage(jsonObject);
    } catch (e) {
      console.log("Non JSON format: ", event.data)
    }

    setServerResponse(event.data);
  }

  const onError = (event) => {
    console.log("Error Occured: ", event.data)
  }

  const onClose = (event) => {
    setServerResponse('');
    setWs("Closed");
  }

  // === PEER CONNECTION CALLBACKS === 

  const onICECandidate = (event) => {

    if (event.candidate) {
      socketRef.current.send(JSON.stringify({
          type: "candidate",
          candidate: event.candidate.candidate,
          mid: event.candidate.sdpMid
        })
      );
    } else {
      console.log("Candidate Exchange Complete")
    }
  }

  const onTrack = (event) => {

    console.log("Recieved Track: ", event.track.id);

    const incomingStream = new MediaStream([event.track]);

    setRemoteStreams((prevStreams) => [...prevStreams, incomingStream]);

  }

  // === MESSAGE HANDLING ===

  const handleMessage = async (jsonObject) => {

    if (jsonObject["type"] === "offer") {
      try{ 
        // wait for remote description to be set and answer to be created
        await pcRef.current.setRemoteDescription(new RTCSessionDescription(jsonObject))
        console.log("Remote description added successfully")

        const answer = await pcRef.current.createAnswer();

        // set local description and send that to the server 
        await pcRef.current.setLocalDescription(answer);

        const localDesc = pcRef.current.currentLocalDescription || pcRef.current.localDescription;

        console.log(answer)

        socketRef.current.send(JSON.stringify({
          type: localDesc.type,
          sdp: localDesc.sdp
        }));

      } catch (e) {
        console.error("Failed to handle offer: ", e);
      }

    } else if (jsonObject["type"] === "candidate") {
      try{

        const iceCandidate = new RTCIceCandidate({
          candidate: jsonObject["candidate"],
          sdpMid: jsonObject["mid"],
        });

        await pcRef.current.addIceCandidate(iceCandidate)
        console.log("Remote candidate added successfully")
      
      } catch (e) {
        console.error("Failure setting remote candidate: ", e)
      }
    }
  }

  // Not usable as of now.
  const sendPing = () => {

    console.log("Sending Ping to Server")

    const msg = {
      type: "ping"
    };

    socketRef.current.send(JSON.stringify(msg))
  
  }

  const initPC = () => {
    
    console.log("Asking Server to Start PC")
    
    const msg = {
      type: "startPC"
    };

    socketRef.current.send(JSON.stringify(msg));

  }

  const closePC = () => {
    
    console.log("Asking Server to Close PC")
    
    const msg = {
      type: "closePC"
    };

    socketRef.current.send(JSON.stringify(msg));

  }

  // === PEER CONNECTION START / STOP HANDLING ===

  const startPeerConnection = () => {
    if ( peerConnectionRef.current === 'Closed' ) {
      pcRef.current = new RTCPeerConnection();

      pcRef.current.addEventListener("icecandidate", onICECandidate);
      pcRef.current.addEventListener("track", onTrack);

      //for(let i=0; i<6; i++) {
      //  pcRef.current.addTransceiver('video', { direction: 'recvonly' });
      //}

      setPc("Open")
      initPC();

    }
  }

  const stopPeerConnection = () => {
    if ( peerConnectionRef.current === 'Open') {
      console.log("Closing Peer Connection")

      pcRef.current?.getReceivers().forEach(receiver => receiver.track?.stop());

      pcRef.current?.removeEventListener("icecandidate", onICECandidate);
      pcRef.current?.removeEventListener("track", onTrack);
      pcRef.current?.close();
      pcRef.current = null;

      setPc("Closed");
      setRemoteStreams([]);
      closePC();

    }
  }

  // === WEBSOCKET START / STOP HANDLING === 

  const startWebsocket = () => {
    if ( webSocketRef.current === 'Closed' ) {
      console.log("Starting Websocket at: ", {webSocketIp})
      //Start Websocket
      socketRef.current = new WebSocket(webSocketIp);

      //Add event listeners
      socketRef.current.addEventListener("open", onOpen);
      socketRef.current.addEventListener("message", onMessage);
      socketRef.current.addEventListener("error", onError);
      socketRef.current.addEventListener("close", onClose);

      //Set websocket ref
      setWs("Open");

    }
  }

  const closeWebsocket = () => {
    if ( webSocketRef.current === 'Open' ) {
      //Log action
      console.log("Closing Websocket")

      //Remove listeners and close
      socketRef.current?.removeEventListener("open", onOpen);
      socketRef.current?.removeEventListener("message", onMessage);
      socketRef.current?.removeEventListener("error", onError);
      socketRef.current?.close();

      //Update state
      setWs("Closed");
      setServerResponse('');
    }
  }

  useEffect(() => {
    const handleTabClose = () => {
      console.log("Tab closing, cleaning up...");

      closeWebsocket();
      stopPeerConnection();

    };

    // Add the event listener when the component mounts
    window.addEventListener('beforeunload', handleTabClose);

    // The cleanup function for the EFFECT itself (React unmounting)
    return () => {
      window.removeEventListener('beforeunload', handleTabClose);
      handleTabClose();

    };
  }, []); 

  // Sub-component to handle the srcObject assignment
  // ALL FRONT-END DESIGN WAS AI GENERATED
  const VideoBox = ({ stream, index }) => {
    const videoRef = useRef();

    useEffect(() => {
      if (videoRef.current && stream) {
        videoRef.current.srcObject = stream;
      }
    }, [stream]);

    return (
      <div style={{
        position: 'relative',
        backgroundColor: '#000',
        borderRadius: '8px',
        overflow: 'hidden',
        border: '2px solid #444',
        aspectRatio: '16/9',
        boxShadow: '0 4px 15px rgba(0,0,0,0.5)'
      }}>
        <video 
          ref={videoRef} 
          autoPlay 
          playsInline 
          muted 
          style={{ width: '100%', height: '100%', objectFit: 'cover' }} 
        />
        <div style={{
          position: 'absolute',
          top: '10px',
          left: '10px',
          backgroundColor: 'rgba(0,0,0,0.6)',
          padding: '2px 8px',
          borderRadius: '4px',
          fontSize: '0.7rem',
          color: '#00ff00',
          fontFamily: 'monospace'
        }}>
          CAM_{index + 1} // LIVE
        </div>
      </div>
    );
  };

  return (
    <div className="App" style={{ 
      fontFamily: 'Inter, system-ui, sans-serif', 
      backgroundColor: '#1a1d23', 
      minHeight: '100vh', 
      color: '#e0e0e0',
      padding: '20px',
      display: 'flex',
      flexDirection: 'column'
    }}>
      
      {/* TOP HEADER: Info & Controls */}
      <header style={{ 
        display: 'flex', 
        justifyContent: 'space-between', 
        alignItems: 'center',
        padding: '0 10px 20px 10px',
        borderBottom: '1px solid #333',
        marginBottom: '20px'
      }}>
        <div>
          <h1 style={{ margin: 0, fontSize: '1.2rem', letterSpacing: '1px', color: '#fff' }}>SYSTEM MONITOR</h1>
          <div style={{ display: 'flex', gap: '15px', marginTop: '5px' }}>
            <StatusBadge label="WS" state={wsState} />
            <StatusBadge label="WebRTC" state={pcState} />
          </div>
        </div>

        <div style={{ display: 'flex', gap: '10px' }}>
          {/* Toggle Button for Websocket */}
          <button 
            onClick={wsState === 'Closed' ? startWebsocket : closeWebsocket}
            style={buttonStyle(wsState === 'Closed' ? '#28a745' : '#dc3545')}
          >
            {wsState === 'Closed' ? 'Connect Server' : 'Disconnect'}
          </button>

          {/* Toggle Button for PeerConnection */}
          <button 
            onClick={pcState === 'Closed' ? startPeerConnection : stopPeerConnection}
            style={buttonStyle(pcState === 'Closed' ? '#007bff' : '#f39c12')}
          >
            {pcState === 'Closed' ? 'Start Feed' : 'Stop Feed'}
          </button>
        </div>
      </header>

      {/* MAIN GRID: The Six Streams */}
      <main style={{
        flex: 1,
        display: 'grid',
        gridTemplateColumns: 'repeat(auto-fit, minmax(400px, 1fr))',
        gap: '20px',
        padding: '10px'
      }}>
        {/* If no streams, show placeholders */}
        {remoteStreams.length === 0 ? (
          [...Array(6)].map((_, i) => (
            <div key={i} style={placeholderStyle}>NO SIGNAL</div>
          ))
        ) : (
          remoteStreams.map((stream, i) => (
            <VideoBox key={i} index={i} stream={stream} />
          ))
        )}
      </main>

      {/* FOOTER: Minimalist Logs */}
      <footer style={{ 
        marginTop: '20px',
        padding: '10px',
        backgroundColor: '#000',
        borderRadius: '6px',
        fontSize: '0.8rem',
        fontFamily: 'monospace',
        color: '#888',
        border: '1px solid #333'
      }}>
        <span style={{ color: '#007bff' }}>{'>'} LOG:</span> {serverResponse || "System standby..."}
      </footer>
    </div>
  );
}

// Helper: Status Badges
const StatusBadge = ({ label, state }) => (
  <span style={{ fontSize: '0.7rem', display: 'flex', alignItems: 'center', gap: '5px' }}>
    <span style={{ color: '#666' }}>{label}:</span>
    <span style={{ 
      color: state === 'Open' ? '#28a745' : '#dc3545',
      fontWeight: 'bold'
    }}>● {state}</span>
  </span>
);

// Styles
const buttonStyle = (color) => ({
  padding: '8px 16px',
  cursor: 'pointer',
  borderRadius: '4px',
  border: 'none',
  backgroundColor: color,
  color: 'white',
  fontWeight: '600',
  fontSize: '0.85rem',
  transition: 'opacity 0.2s'
});

const placeholderStyle = {
  backgroundColor: '#121418',
  borderRadius: '8px',
  border: '2px dashed #333',
  display: 'flex',
  justifyContent: 'center',
  alignItems: 'center',
  aspectRatio: '16/9',
  color: '#444',
  fontSize: '0.9rem',
  fontWeight: 'bold'
};

export default App;