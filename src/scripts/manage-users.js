const { PrismaClient } = require('@prisma/client');
const bcrypt = require('bcrypt');
const prisma = new PrismaClient();

async function createUser(username, password, fullName = username, role = "user") {
  try {
    const hashedPassword = await bcrypt.hash(password, 12);
    const user = await prisma.user.create({
      data: {
        username,
        password: hashedPassword,
        fullName,
        role,
      },
    });
    console.log(`User created successfully: ${user.username}`);
    return user;
  } catch (error) {
    console.error('Error creating user:', error.message);
    throw error;
  }
}

async function removeUser(username) {
  try {
    const user = await prisma.user.delete({
      where: {
        username: username
      }
    });
    console.log(`User removed successfully: ${user.username}`);
    return user;
  } catch (error) {
    console.error('Error removing user:', error.message);
    throw error;
  }
}

async function checkUser(username) {
  try {
    const user = await prisma.user.findUnique({
      where: {
        username: username
      }
    });
    
    if (user) {
      console.log('User found:');
      console.log({
        username: user.username,
        fullName: user.fullName,
        role: user.role,
        id: user.id,
        password: '[HIDDEN]'
      });
      return user;
    } else {
      console.log('No user found with username:', username);
      return null;
    }
  } catch (error) {
    console.error('Error checking user:', error.message);
    throw error;
  }
}

async function verifyCredentials(username, password) {
  try {
    const user = await prisma.user.findUnique({
      where: {
        username: username
      }
    });

    if (!user) {
      console.log('No user found with username:', username);
      return false;
    }

    const validPassword = await bcrypt.compare(password, user.password);
    console.log('Password match:', validPassword);
    return validPassword;
  } catch (error) {
    console.error('Error verifying credentials:', error.message);
    throw error;
  }
}

async function listUsers() {
  try {
    const users = await prisma.user.findMany({
      select: {
        id: true,
        username: true,
        fullName: true,
        role: true,
        createdAt: true,
      }
    });
    console.log('\nList of users:');
    console.table(users);
    return users;
  } catch (error) {
    console.error('Error listing users:', error.message);
    throw error;
  }
}

// Command line interface
async function main() {
  const command = process.argv[2];
  const username = process.argv[3];
  const password = process.argv[4];

  try {
    switch (command) {
      case 'create':
        if (!username || !password) {
          console.log('Usage: node manage-users.js create <username> <password>');
          process.exit(1);
        }
        await createUser(username, password);
        break;

      case 'remove':
        if (!username) {
          console.log('Usage: node manage-users.js remove <username>');
          process.exit(1);
        }
        await removeUser(username);
        break;

      case 'check':
        if (!username) {
          console.log('Usage: node manage-users.js check <username>');
          process.exit(1);
        }
        await checkUser(username);
        break;

      case 'verify':
        if (!username || !password) {
          console.log('Usage: node manage-users.js verify <username> <password>');
          process.exit(1);
        }
        const isValid = await verifyCredentials(username, password);
        console.log('Credentials valid:', isValid);
        break;

      case 'list':
        await listUsers();
        break;

      default:
        console.log('Usage:');
        console.log('  Create user: node manage-users.js create <username> <password>');
        console.log('  Remove user: node manage-users.js remove <username>');
        console.log('  Check user: node manage-users.js check <username>');
        console.log('  Verify credentials: node manage-users.js verify <username> <password>');
        console.log('  List all users: node manage-users.js list');
    }
  } catch (error) {
    console.error('Operation failed:', error.message);
    process.exit(1);
  } finally {
    await prisma.$disconnect();
  }
}

main();
