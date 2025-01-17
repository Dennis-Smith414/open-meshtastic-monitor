export default function Home() {
  return (
    <div className="min-h-screen bg-[url('/topo-map.webp')] bg-cover bg-center flex items-center justify-center">
      <div className="bg-white/10 backdrop-blur-lg rounded-2xl p-8 w-full max-w-sm mx-4 shadow-xl">
        {/* Header */}
        <div className="text-center mb-8">
          <h1 className="text-3xl font-bold text-emerald-500 mb-2">Meshtastic Web</h1>
          <p className="text-emerald-600">Secure Mesh Network Access</p>
        </div>

        {/* Login Form */}
        <form className="space-y-6">
          <div>
            <input
              type="text"
              className="w-full px-4 py-3 bg-white/80 border border-emerald-300 rounded-lg 
                       text-emerald-900 placeholder-emerald-500 focus:outline-none focus:ring-2 
                       focus:ring-emerald-500 focus:border-transparent transition-all"
              placeholder="Username"
            />
          </div>

          <div>
            <input
              type="password"
              className="w-full px-4 py-3 bg-white/80 border border-emerald-300 rounded-lg 
                       text-emerald-900 placeholder-emerald-500 focus:outline-none focus:ring-2 
                       focus:ring-emerald-500 focus:border-transparent transition-all"
              placeholder="Password"
            />
          </div>

          <button
            type="submit"
            className="w-full bg-emerald-600 text-white py-3 px-4 rounded-lg font-medium
                     hover:bg-emerald-700 focus:outline-none focus:ring-2 focus:ring-emerald-500 
                     focus:ring-offset-2 focus:ring-offset-emerald-100 transition-colors"
          >
            Login
          </button>
        </form>
      </div>
    </div>
  );
}