using SpotifyAuthExample.Controllers;
using System;
using System.Threading.Tasks; // �������� ��� ���������

class Program
{
    static async Task Main(string[] args) // �������� �������� �� async
    {
        var spotifyController = new SpotifyController();

        // ������������� ������������ �� �������� �������������� Spotify
        var authUri = await spotifyController.Authorize(); // ����������� await

        Console.WriteLine("������������� ������������ �� ��������� �������� ��� ��������������:");
        Console.WriteLine(authUri);

        // ��������� ������ �� Spotify � �������� access token
        Console.Write("������� ��� ����������� (authorization code) ����� �������� ��������������: ");
        string code = Console.ReadLine();

        var userInfo = await spotifyController.Callback(code);

        if (!string.IsNullOrEmpty(userInfo))
        {
            Console.WriteLine("������ ������������ Spotify:");
            Console.WriteLine(userInfo);
        }
        else
        {
            Console.WriteLine("�� ������� �������� ������ ������������.");
        }

        Console.ReadLine();
    }
}
