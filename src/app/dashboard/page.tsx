// pages/dashboard.tsx
"use client"
import { useState, useEffect } from 'react';
import mqtt from 'mqtt';

// Types
interface User {
  username: string;
  role: string;
  fullName: string;
}

interface NodeData {
  from: string;
  name?: string;
  battery?: {
    level: number;
    voltage: number;
  };
  position?: {
    latitude: number;
    longitude: number;
    altitude?: number;
  };
  lastUpdate: number;
}

interface Message {
  id: number;
  timestamp: number;
  from: string;
  text: string;
}

export default function DashboardPage() {
  const [user, setUser] = useState<User | null>(null);
  const [nodes, setNodes] = useState<Record<string, NodeData>>({});
  const [messages, setMessages] = useState<Message[]>([]);
  const [connectionStatus, setConnectionStatus] = useState<string>('Disconnected');
  const [client, setClient] = useState<mqtt.MqttClient | null>(null);

  useEffect(() => {
    // Handle authentication
    const userStr = sessionStorage.getItem('user');
    if (!userStr) {
      window.location.href = '/';
      return;
    }
    setUser(JSON.parse(userStr));

    // Set up MQTT connection
    const mqttClient = mqtt.connect('ws://localhost:9001', {
      clientId: `nextjs_${Math.random().toString(16).slice(2, 8)}`,
      clean: true,
      connectTimeout: 4000,
      reconnectPeriod: 1000,
    });

    mqttClient.on('connect', () => {
      console.log('Connected to MQTT broker');
      setConnectionStatus('Connected');
      
      const topics = [
        'meshtastic/messages/#',
        'meshtastic/nodes/#',
        'meshtastic/device/#'
      ];
      
      topics.forEach(topic => {
        mqttClient.subscribe(topic, (err) => {
          if (err) console.error(`Subscription error for ${topic}:`, err);
        });
      });
    });

    mqttClient.on('error', (err) => {
      console.error('MQTT Error:', err);
      setConnectionStatus('Error');
    });

    mqttClient.on('offline', () => {
      setConnectionStatus('Offline');
    });

    mqttClient.on('message', (topic, payload) => {
      try {
        const data = JSON.parse(payload.toString());
        
        if (topic.includes('messages')) {
          handleMessage(data);
        } else if (topic.includes('nodes')) {
          handleNodeUpdate(data);
        }
      } catch (e) {
        console.error('Error parsing message:', e);
      }
    });

    setClient(mqttClient);

    return () => {
      if (mqttClient) {
        mqttClient.end();
      }
    };
  }, []);

  const handleMessage = (data: any) => {
    setMessages(prev => [...prev, {
      id: Date.now(),
      timestamp: data.timestamp || Date.now() / 1000,
      from: data.from,
      text: data.payload,
    }].slice(-100)); // Keep last 100 messages
  };

  const handleNodeUpdate = (data: any) => {
    setNodes(prev => ({
      ...prev,
      [data.from]: {
        ...prev[data.from],
        ...data,
        lastUpdate: Date.now()
      }
    }));
  };

  const handleLogout = () => {
    if (client) {
      client.end();
    }
    sessionStorage.removeItem('user');
    window.location.href = '/';
  };

  if (!user) {
    return <div>Loading...</div>;
  }

  return (
    <div className="min-h-screen bg-[url('/topo-map.webp')]">
      {/* Navigation Bar */}
      <div className="max-w-6xl mx-auto px-4">
        <div className="flex justify-between items-center py-4">
          <div className="text-xl font-bold text-emerald-600">
            Open Meshtastic Monitor (Alpha)
          </div>
          <div className="flex items-center space-x-4">
            <span className="text-emerald-600">Welcome, {user.fullName}</span>
            <span className="text-sm text-emerald-500">MQTT: {connectionStatus}</span>
            <button
              onClick={handleLogout}
              className="px-4 py-2 bg-emerald-600 text-white rounded-lg hover:bg-emerald-700"
            >
              Logout
            </button>
          </div>
        </div>
      </div>

      {/* Main Content */}
      <div className="max-w-6xl mx-auto px-4 py-8">
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
          {/* Node Status Panel */}
          <div className="bg-white/90 backdrop-blur-sm rounded-lg shadow-lg p-6">
            <h2 className="text-xl font-bold text-emerald-700 mb-4">Network Nodes</h2>
            <div className="space-y-4">
              {Object.entries(nodes).map(([nodeId, node]) => (
                <div key={nodeId} className="border rounded-lg p-4 shadow-sm bg-white">
                  <div className="flex justify-between items-start">
                    <div>
                      <h3 className="font-medium text-emerald-600">{node.name || node.from}</h3>
                      <div className="text-sm text-gray-600 mt-1">
                        {node.battery && (
                          <p>Battery: {node.battery.level}% ({node.battery.voltage}V)</p>
                        )}
                      </div>
                    </div>
                    <div className="text-right text-sm text-gray-500">
                      Last seen: {timeAgo(node.lastUpdate)}
                    </div>
                  </div>
                  {node.position && (
                    <div className="mt-2 text-black text-sm">
                      <p>Location: {formatCoordinates(node.position.latitude, node.position.longitude)}</p>
                      {node.position.altitude && (
                        <p>Altitude: {node.position.altitude.toFixed(1)}m</p>
                      )}
                    </div>
                  )}
                </div>
              ))}
              {Object.keys(nodes).length === 0 && (
                <div className="text-gray-500 text-center py-4">
                  No nodes connected
                </div>
              )}
            </div>
          </div>

          {/* Messages Panel */}
          <div className="bg-white/90 backdrop-blur-sm rounded-lg shadow-lg p-6">
            <h2 className="text-xl font-bold text-emerald-700 mb-4">Messages</h2>
            <div className="space-y-3 max-h-[600px] overflow-y-auto">
              {messages.map((msg) => (
                <div key={msg.id} className="border rounded-lg p-3 bg-white">
                  <div className="flex justify-between">
                    <span className="font-medium text-emerald-600">
                      {(nodes[msg.from]?.name) || msg.from}
                    </span>
                    <span className="text-sm text-gray-500">
                      {new Date(msg.timestamp * 1000).toLocaleString()}
                    </span>
                  </div>
                  <p className="mt-1 text-gray-700">{msg.text}</p>
                </div>
              ))}
              {messages.length === 0 && (
                <div className="text-gray-500 text-center py-4">
                  No messages yet
                </div>
              )}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

// Utility functions
function timeAgo(timestamp: number): string {
  const seconds = Math.floor((Date.now() - timestamp) / 1000);
  
  const intervals = {
    year: 31536000,
    month: 2592000,
    week: 604800,
    day: 86400,
    hour: 3600,
    minute: 60,
    second: 1
  };

  for (const [unit, secondsInUnit] of Object.entries(intervals)) {
    const interval = Math.floor(seconds / secondsInUnit);
    if (interval >= 1) {
      return `${interval} ${unit}${interval === 1 ? '' : 's'} ago`;
    }
  }
  return 'just now';
}

function formatCoordinates(lat: number, lon: number): string {
  if (!lat || !lon) return 'Unknown';
  return `${lat.toFixed(6)}°, ${lon.toFixed(6)}°`;
}