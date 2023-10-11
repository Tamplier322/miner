using SpotifyAuthExample.Controllers;
using System;
using System.Threading.Tasks; // Добавьте эту директиву

class Program
{
    static async Task Main(string[] args) // Обратите внимание на async
    {
        var spotifyController = new SpotifyController();

        // Перенаправьте пользователя на страницу аутентификации Spotify
        var authUri = await spotifyController.Authorize(); // Используйте await

        Console.WriteLine("Перенаправьте пользователя на следующую страницу для аутентификации:");
        Console.WriteLine(authUri);

        // Дождитесь ответа от Spotify и получите access token
        Console.Write("Введите код авторизации (authorization code) после успешной аутентификации: ");
        string code = Console.ReadLine();

        var userInfo = await spotifyController.Callback(code);

        if (!string.IsNullOrEmpty(userInfo))
        {
            Console.WriteLine("Данные пользователя Spotify:");
            Console.WriteLine(userInfo);
        }
        else
        {
            Console.WriteLine("Не удалось получить данные пользователя.");
        }

        Console.ReadLine();
    }
}
