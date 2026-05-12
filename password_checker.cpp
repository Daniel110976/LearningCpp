#include <iostream>
#include <string>
#include <cctype>

bool hasUppercase(const std::string& password)  {
    for (char ch : password) {
        if (std::isupper(ch))   
            return true;
    }
    return false;
}
bool hasLowercase(const std::string& password)  {
    for (char ch : password)  {
        if (std::islower(ch))   
           return true;
    }
    return false;
}
bool hasDigit(const std::string& password)  {
    for (char ch : password)  {
        if(std::isdigit(ch))
           return true;
    }
    return false;
}
bool hasSpecialChar(const std::string& password)  {
    for (char ch : password)  {
        if(std::ispunct(ch))
          return true;
    }
    return false;
}
std::string checkStrength(const std::string& password)  {
    int score = 0;
    if (password.length() >= 8)  score++;
    if (password.length() >= 12)  score++;
    if (hasUppercase(password))  score++;
    if (hasLowercase(password))  score++;
    if (hasDigit(password))  score++;
    if (hasSpecialChar(password))  score++;

    if (score <= 2) return "Weak";
    if (score <= 4) return "Moderate";
    return "Strong";
}

int main()  {
    std::string password;

    std::cout << "Enter a password to check: ";
    std::cin >> password;

    std::cout << "\n--- Password Report ---\n";
    std::cout << "Length    : " << password.length() << " characters\n";
    std::cout << "Has uppercase    : " <<(hasUppercase(password) ? "Yes" : "No") << "\n";
    std::cout << "Has lowercase    : " <<(hasLowercase(password) ? "Yes" : "No") << "\n";
    std::cout << "Has digit    : " <<(hasDigit(password) ? "Yes" : "No") << "\n";
    std::cout << "Has special    : " <<(hasSpecialChar(password) ? "Yes" : "No") << "\n";
    std::cout << "\nStrength: " << checkStrength(password) << "\n";

    return 0;
} 
