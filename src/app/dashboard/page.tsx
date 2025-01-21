"use client"

import { useState, useEffect } from 'react';

export default function DashboardPage() {
  const [user, setUser] = useState<{
    username: string;
    role: string;
    fullName: string;
  } | null>(null);

  useEffect(() => {
    // Get user info from session storage
    const userStr = sessionStorage.getItem('user');
    if (!userStr) {
      window.location.href = '/';
      return;
    }
    setUser(JSON.parse(userStr));
  }, []);

  const handleLogout = () => {
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
            <div className="text-xl font-bold text-emerald-600">Open Meshtastic Monitor (Alpha)</div>
            <div className="flex items-center space-x-4">
              <span className="text-emerald-600">Welcome, {user.fullName}</span>
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
        <div className="bg-white rounded-lg shadow-lg p-6">
          <h1 className="text-2xl font-bold text-emerald-700 mb-4">
            Dashboard
          </h1>
          <p className="text-gray-600">
            You are successfully logged in as: {user.role}
          </p>
          {/* Features to be added */}
        </div>
      </div>
    </div>
  );
}