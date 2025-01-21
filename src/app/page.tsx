"use client"
import { useState } from 'react';
import { useRouter } from 'next/navigation';

export default function Home() {
  const router = useRouter();
  const [credentials, setCredentials] = useState({
    username: '',
    password: ''
  });
  const [error, setError] = useState('');
  const [isLoading, setIsLoading] = useState(false);

  const handleSubmit = async (e: React.FormEvent<HTMLFormElement>) => {
    e.preventDefault();
    setIsLoading(true);
    setError('');

    try {
      const response = await fetch('/api/auth', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(credentials),
      });

      const data = await response.json();

      if (response.ok && data.user) {
        // Store user data in sessionStorage
        sessionStorage.setItem('user', JSON.stringify(data.user));
        // Redirect to dashboard
        router.push('/dashboard');
      } else {
        setError(data.error || 'Login failed');
      }
    } catch (error) {
      console.error('Error details:', error);
      setError('Login failed. Please try again.');
    } finally {
      setIsLoading(false);
    }
  };

  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    setCredentials({
      ...credentials,
      [e.target.name]: e.target.value
    });
  };

  return (
    <div className="min-h-screen bg-[url('/topo-map.webp')] bg-cover bg-center flex items-center justify-center">
      <div className="bg-white/10 backdrop-blur-lg rounded-2xl p-8 w-full max-w-sm mx-4 shadow-xl">
        <div className="text-center mb-8">
          <h1 className="text-3xl font-bold text-emerald-500 mb-2">Open Meshtastic Monitor</h1>
          <p className="text-emerald-600">A log of your Meshtastic Network</p>
        </div>

        <form className="space-y-6" onSubmit={handleSubmit}>
          {error && (
            <div className="text-red-500 text-sm text-center">{error}</div>
          )}

          <div>
            <input
              type="text"
              name="username"
              value={credentials.username}
              onChange={handleChange}
              className="w-full px-4 py-3 bg-white/80 border border-emerald-300 rounded-lg text-emerald-900 placeholder-emerald-500 focus:outline-none focus:ring-2 focus:ring-emerald-500 focus:border-transparent transition-all"
              placeholder="Username"
            />
          </div>

          <div>
            <input
              type="password"
              name="password"
              value={credentials.password}
              onChange={handleChange}
              className="w-full px-4 py-3 bg-white/80 border border-emerald-300 rounded-lg text-emerald-900 placeholder-emerald-500 focus:outline-none focus:ring-2 focus:ring-emerald-500 focus:border-transparent transition-all"
              placeholder="Password"
            />
          </div>

          <button
            type="submit"
            disabled={isLoading}
            className="w-full bg-emerald-600 text-white py-3 px-4 rounded-lg font-medium hover:bg-emerald-700 focus:outline-none focus:ring-2 focus:ring-emerald-500 focus:ring-offset-2 focus:ring-offset-emerald-100 transition-colors disabled:opacity-50"
          >
            {isLoading ? 'Logging in...' : 'Login'}
          </button>
        </form>
      </div>
    </div>
  );
}