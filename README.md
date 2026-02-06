# Multihreaded-Web-Crawler

TLDR: Web Crawlers naturally form a pipeline, which can be roughly divided into 1. Deciding which URL to visit next 2. Fetch data 3. Parse/extract 4. store/index results. Doing this sequentially can be quite slow so multithreading can be employed to index several websites simultaenously.

Current Progress:
- Implemented Bounded Blocking Queue
    - Utilizing '''<std::mutex>''' to ensure thread safety
    - Limiting workers prevents overloading and relieves back pressure, assuming fetching is faster than parsing/storing
    - **Needs Testing**
- URL Frontier
    - When the workers are visiting different URLs, it is crucial we implement URL deduplication
    - We normalize then insert the shortened URL into our visited set
    - Uses '''<std::shared_mutex>''' since we are doing mostly read-only operations and simultaneous writes of the same content are rare and already accounted for with set uniqueness
    - **Needs function implementation in a .cpp file**

Current Thoughts:
- Realizing I'm going to have to deal with validating/normalizing URLs somewhere, this part isn't super non-trivial it's just I have to figure out where to put it
- How the hell am I gonna deal with page parsing?
- Gonna have to deal with the configuration (timeout, size of Bounded Queues), but that will have to be done with unit testing later

Resources I find/found useful:
- Web Crawlers: https://bytebytego.com/courses/system-design-interview/design-a-web-crawler 
- Bounded Queue: https://leetcode.com/problems/design-bounded-blocking-queue/description/ (I'm serious)
- Concurrency: https://www.youtube.com/playlist?list=PLvv0ScY6vfd_ocTP2ZLicgqKnvq50OCXM (Mike Shah is the GOAT)
- CPP in General: https://www.learncpp.com/ 
- URL Frontier: https://nlp.stanford.edu/IR-book/html/htmledition/the-url-frontier-1.html
- Rate Limiter: https://bytebytego.com/courses/system-design-interview/design-a-rate-limiter 

To-Do:
**Crucial**
- Per-Domain Rate Limiter: So I literally don't DDOS a website, called "politeness"
- Thread-Safe LRU Cache: So I can lookup DNS and domain metadata
- Fetcher/Parser/Indexer
- Look into:
    - OpenSSL SHA256 Hashing (I know briefly because Git uses it for object hashing)
    - libcurl for HTTP Fetching (idk)
    - Different types of parsing (gumbo-parser or regex)
    - Database sharding (idk)

**Nice-to-Have**
- Metrics/Stats: p50/p90 latencies
